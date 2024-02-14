# Audit Plugin Installation Instructions

## Install OpenSearch

_For native installation, this needs to be installed on Ubuntu 20.04._

1. Install Docker

   Set up Docker's Apt repository.

   ```bash
   sudo apt install ca-certificates curl gnupg
   sudo install --directory --mode=0755 /etc/apt/keyrings
   curl --fail --location --show-error --silent https://download.docker.com/linux/ubuntu/gpg \
     | sudo gpg --dearmor --output /etc/apt/keyrings/docker.gpg
   sudo chmod a+r /etc/apt/keyrings/docker.gpg

   # Add the repository to Apt sources:
   printf \
       'deb [arch=%s signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu %s stable\n' \
       "$(dpkg --print-architecture)" \
       "$(. /etc/os-release && echo "$VERSION_CODENAME")" \
     | sudo tee /etc/apt/sources.list.d/docker.list \
     > /dev/null
   ```

   Install the Docker packages.

   ```bash
   sudo apt update
   sudo apt install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
   ```

1. Configure host for OpenSearch

   Disable memory paging and swapping.

   ```bash
    sudo swapoff --all
   ```

    Edit the sysctl config file, `/etc/sysctl.conf`, setting the max map count to 262,144.

    ```bash
    echo vm.max_map_count=262144 | sudo tee --append /etc/sysctl.conf > /dev/null
    ```

    Reload the kernel parameters.

    ```bash
   sudo sysctl --load
    ```

1. Get the All-in-One Compose file

   ```bash
   wget \
     https://raw.githubusercontent.com/opensearch-project/documentation-website/2.9/assets/examples/docker-compose.yml
   ```

1. Start OpenSearch

   ```bash
   sudo docker compose up --detach
   ```

## Add AMQP 1.0 support to Data Store's RabbitMQ Broker

The AMQP 1.0 plugin needs to be configured for the RabbitMQ broker.

1. Enable AMQP 1.0 plugins

   ```bash
   sudo rabbitmq-plugins enable rabbitmq_amqp1_0
   ```

1. Configure AMQP 1.0 plugin

   ```bash
   cat <<EOF | sudo tee --append /etc/rabbitmq/rabbitmq.conf > /dev/null
   amqp1_0.default_vhost = $DS_VHOST
   amqp1_0.convert_app_props_to_amqp091_headers = true
   EOF
   ```

1. Restart broker

   ```bash
   sudo systemctl restart rabbitmq-server
   ```

## Install LogStash

1. Add Elastic Apt repository.

   ```bash
   curl --fail --location --show-error --silent https://artifacts.elastic.co/GPG-KEY-elasticsearch \
     | sudo gpg --dearmor --output /usr/share/keyrings/elastic-keyring.gpg
   sudo apt install apt-transport-https
   echo \
       'deb [signed-by=/usr/share/keyrings/elastic-keyring.gpg] https://artifacts.elastic.co/packages/8.x/apt stable main' \
     | sudo tee --append /etc/apt/sources.list.d/elastic-8.x.list \
     > /dev/null
   ```

1. Install logstash

   ```bash
   sudo apt update
   sudo apt install logstash
   ```

1. Install OpenSearch and RabbitMQ logstash plugins

   ```bash
   sudo /usr/share/logstash/bin/logstash-plugin install \
     logstash-output-opensearch logstash-input-rabbitmq
   ```

1. Configure pipeline

   ```bash
   sudo cat <<EOF | tee /etc/logstash/conf.d/irods.conf > /dev/null
   input {
      rabbitmq {
         host => "$RABBITMQ_HOST"
         vhost => "$DS_VHOST"
         user => "$LOGSTASH_RABBITMQ_USERNAME"
         password => "$LOGSTASH_RABBITMQ_PASSWORD"
         exchange => "irods"
         key => "audit"
         codec => plain { charset => "ISO-8859-1" }
      }
   }

   filter {
      # Remove AMQP 1.0 header
      mutate { gsub => [ "message", "[^{]*(.*)", "\1" ] }

      json {
         source => "message"
         target => "message"
      }

      # Replace @timestamp with the timestamp stored in message field @timestamp
      date { match => [ "[message][@timestamp]", "UNIX_MS" ] }

      # TODO: Verify which fields need to be converted to integers for iRODS 4.3.1
      mutate {
         convert => {
            "[message][file_size]" => "integer"
            "[message][data_size]" => "integer"
         }
      }

      # Convert nameless int fields to integers and removed ERROR fields
      ruby {
         code => '
            event.get("message").to_hash.each { |k, v|
               if ( k =~ /^int(__[0-9]+)?$/ )
                  event.set("[message][" + k + "]", v.to_i)
               elsif ( k =~ /^ERROR(__[0-9]+)?$/ )
                  event.remove("[message][" + k + "]")
               end
            }
         '
      }
   }

   output {
      opensearch {
         hosts => [ "https://${OPENSEARCH_HOST}:9200" ]
         user => "$LOGSTASH_OPENSEARCH_USERNAME"
         password => "$LOGSTASH_OPENSEARCH_PASSWORD"
         index => "irods-audit-%{+YYYY.MM.dd}"
      }
   }
   EOF
   ```

1. Run logstash

   ```bash
   sudo systemctl enable logstash
   sudo systemctl start logstash
   ```

## Add iRODS Audit Plugin to Data Store iRODS catalog service provider

1. Install the iRODS audit plugin

   ```bash
   sudo apt install irods_rule-engine-plugin-audit-amqp
   ```

1. Configure iRODS to use audit plugin

   Prepend the audit plugin configuration to the list of rule engine plugin configurations.

   ```bash
   jq --from-file /dev/stdin /etc/irods/server_config.json \
   <<JQ | sudo sponge /etc/irods/server_config.json
   .plugin_configuration.rule_engines =
      [
         {
            instance_name: "irods_rule_engine_plugin-audit_amqp-instance",
            plugin_name: "irods_rule_engine_plugin-audit_amqp",
            plugin_specific_configuration : {
               amqp_location:
                  "amqp://$IRODS_RABBITMQ_USERNAME:$IRODS_RABBITMQ_PASSWORD@$RABBITMQ_HOST:5672",
               amqp_topic: "/exchange/irods/audit",
               pep_regex_to_match: "pep_.+"
            }
         }
      ]
      + .plugin_configuration.rule_engines
   JQ
   ```

1. Restart iRODS

   ```bash
   sudo systemctl restart irods
   ```
