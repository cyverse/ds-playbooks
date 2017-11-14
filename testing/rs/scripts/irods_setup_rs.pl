#
# Perl

#
# Create the database tables and finish the iRODS installation
#
# Usage is:
#       perl irods_setup.pl [options]
#
use warnings;

use File::Spec;
use Cwd;

$scripttoplevel = "/var/lib/irods";
$userIrodsDir = File::Spec->catdir($scripttoplevel, ".irods" );

########################################################################
#
# Confirm execution from the top-level iRODS directory.
#
$IRODS_HOME = cwd( );   # Might not be actual iRODS home.  Fixed below.

########################################################################
#
# Load support scripts.
#

my $perlScriptsDir = File::Spec->catdir( $IRODS_HOME, "scripts", "perl" );

require File::Spec->catfile( $perlScriptsDir, "utils_paths.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_file.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_platform.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_config.pl" );

########################################################################
#
# Check script usage.
#

setPrintVerbose( 1 );

########################################################################
#
# Load and validate server configuration files.
#

load_server_config("/etc/irods/server_config.json");

# Override variable loaded from server config
$IRODS_HOST = $ENV{"IRODS_HOST"};

my $totalSteps  = 0;
my $currentStep = 0;


########################################################################
#
# Setup steps.
#
#       All of these functions execute individual steps in the
#       database setup.  They all output error messages themselves
#       and may exit the script directly.
#

#
# @brief        Configure iRODS.
#
# Update the iRODS server_config.json file.
#
# Output error messages and exit on problems.
#
sub configureIrodsServer
{
  ++$currentStep;
  printSubtitle( "\n" );
  printSubtitle( "Step $currentStep of $totalSteps:  Configuring iRODS server...\n" );

  # Update/set the iRODS server configuration.
  #       Tell iRODS the database host (often this host),
  #       the database administrator's name, and their MD5
  #       scrambled database password.
  #
  #       If this script is run more than once, then the file
  #       may already be updated.  It's so small there is
  #       little point in checking for this first.  Just
  #       overwrite it with the correct values.

  my $host = $IRODS_ICAT_HOST;

  $serverConfigFile = "/etc/irods/server_config.json";

  my %svr_variables = (
  "icat_host",        $host,
  "zone_name",        $ZONE_NAME,
  "zone_port",        $IRODS_PORT + 0, # convert to integer
  "zone_user",        $IRODS_ADMIN_NAME,
  "zone_auth_scheme", "native" );
  printStatus( "Updating $serverConfigFile...\n" );
  $status = update_json_configuration_file( $serverConfigFile, %svr_variables );

  if ( $status == 0 )
  {
    printError( "\nInstall problem:\n" );
    printError( "    Updating of iRODS $serverConfigFile failed.\n" );
    cleanAndExit( 1 );
  }
  chmod( 0600, $serverConfigFile );

  # User's iRODS configuration directory doesn't exist yet.
  # This is needed for iinit to be able to save the pw file.
  mkdir( $userIrodsDir, 0700 );

  return;
}


#
# @brief        Configure iRODS user.
#
# Update the user's iRODS environment file and set their default
# resource.  Test that it works.
#
# This is made more complicated by a lot of error checking to
# see if each of these tasks has already been done.  If a task
# has been done, we skip it and try the next one.  This insures
# that we'll pick up wherever we left off from a prior failure.
#
# Output error messages and exit on problems.
#
sub configureIrodsUser
{
  ++$currentStep;
  printSubtitle( "\n" );
  printSubtitle( "Step $currentStep of $totalSteps:  Configuring iRODS user and starting server...\n" );

  # Create a irods_environment.json file for the user, if needed.
  printStatus( "Updating iRODS user's ~/.irods/irods_environment.json...\n" );

  # populate the irods environment for this server instance
  printToFile( $userIrodsFile,
    "{\n" .
    "    \"irods_host\": \"$IRODS_HOST\",\n" .
    "    \"irods_port\": $IRODS_PORT,\n" .
    "    \"irods_default_resource\": \"$RESOURCE_NAME\",\n" .
    "    \"irods_home\": \"/$ZONE_NAME/home/$IRODS_ADMIN_NAME\",\n" .
    "    \"irods_cwd\": \"/$ZONE_NAME/home/$IRODS_ADMIN_NAME\",\n" .
    "    \"irods_user_name\": \"$IRODS_ADMIN_NAME\",\n" .
    "    \"irods_zone_name\": \"$ZONE_NAME\",\n" .
    "    \"irods_client_server_negotiation\": \"request_server_negotiation\",\n" .
    "    \"irods_client_server_policy\": \"CS_NEG_REFUSE\",\n" .
    "    \"irods_encryption_key_size\": 32,\n" .
    "    \"irods_encryption_salt_size\": 8,\n" .
    "    \"irods_encryption_num_hash_rounds\": 16,\n" .
    "    \"irods_encryption_algorithm\": \"AES-256-CBC\",\n" .
    "    \"irods_default_hash_scheme\": \"SHA256\",\n" .
    "    \"irods_match_hash_policy\": \"compatible\",\n" .
    "    \"irods_server_control_plane_port\": $CONTROL_PLANE_PORT,\n" .
    "    \"irods_server_control_plane_key\": \"$CONTROL_PLANE_KEY\",\n" .
    "    \"irods_server_control_plane_encryption_num_hash_rounds\": $CONTROL_PLANE_NUM_HASH_ROUNDS,\n" .
    "    \"irods_server_control_plane_encryption_algorithm\": \"$CONTROL_PLANE_ALGORITHM\",\n" .
    "    \"irods_maximum_size_for_single_buffer_in_megabytes\": 32,\n" .
    "    \"irods_default_number_of_transfer_threads\": 4,\n" .
    "    \"irods_transfer_buffer_size_for_parallel_transfer_in_megabytes\": 4\n" .
    "}\n"
  );

  chmod( 0600, $userIrodsFile );
}


#
# @brief        Exit the script abruptly.
#
# Print a final message, then close the log and exit.
#
# @param        $code   the exit code
#
sub cleanAndExit($)
{
  my ($code) = @_;

  printError( "\nAbort.\n" );

  exit( $code );
}


########################################################################
#
# Finish setting up iRODS.
#

# There is a database.  We need to configure it.
$totalSteps  = 2;
$currentStep = 0;

# 1.  Configure
configureIrodsServer( );

# 2.  Configure the iRODS user account.
configureIrodsUser( );

# Done.
exit( 0 );
