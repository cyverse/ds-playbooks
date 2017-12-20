#
# Perl

#
# Create the database tables and finish the iRODS installation
#
# Usage is:
#       perl irods_setup.pl [options]
#
use warnings;
no warnings 'once';

use Cwd;
use File::Spec;

########################################################################
#
# Confirm execution from the top-level iRODS directory.
#

$IRODS_HOME = cwd( );   # Might not be actual iRODS home.  Fixed below.

$userIrodsDir = "/var/lib/irods/.irods";

########################################################################
#
# Load support scripts.
#

my $perlScriptsDir = File::Spec->catdir( $IRODS_HOME, "scripts", "perl" );

require File::Spec->catfile( $perlScriptsDir, "utils_config.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_file.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_paths.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_platform.pl" );

########################################################################
#
# Initialize global flags.
#

setPrintVerbose( 1 );

########################################################################
#
# Load and validate server configuration files.
#

load_server_config("/etc/irods/server_config.json");
load_database_config("/etc/irods/database_config.json");

$DB_NAME = $ENV{"DB_NAME"};
$IRODS_HOST = $ENV{"IRODS_HOST"};

$DATABASE_TYPE           = $ARGV[0];
$DATABASE_HOST           = $ARGV[1];
$DATABASE_PORT           = $ARGV[2];
$DATABASE_ADMIN_NAME     = $ARGV[3];
$DATABASE_ADMIN_PASSWORD = $ARGV[4];


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
  # Update/set the iRODS server configuration.
  #       Tell iRODS the database host (often this host),
  #       the database administrator's name, and their MD5
  #       scrambled database password.
  #
  #       If this script is run more than once, then the file
  #       may already be updated.  It's so small there is
  #       little point in checking for this first.  Just
  #       overwrite it with the correct values.

  my $host = "localhost";

  $serverConfigFile = "/etc/irods/server_config.json";
  $databaseConfigFile = "/etc/irods/database_config.json";

  my %svr_variables = (
    "icat_host",        $host,
    "zone_name",        $ZONE_NAME,
    "zone_port",        $IRODS_PORT + 0,    # convert to integer
    "zone_user",        $IRODS_ADMIN_NAME,
    "zone_auth_scheme", "native" );

  printStatus( "Updating $serverConfigFile...\n" );
  $status = update_json_configuration_file( $serverConfigFile, %svr_variables );

  if ( $status == 0 )
  {
    printError( "\nInstall problem:\n" );
    printError( "    Updating of iRODS $serverConfigFile failed.\n" );
    printError( "        ", $output );
    cleanAndExit( 1 );
  }

  chmod( 0600, $serverConfigFile );

  my %db_variables = (
    "catalog_database_type", $DATABASE_TYPE,
    "db_username",           $DATABASE_ADMIN_NAME,
    "db_password",           $DATABASE_ADMIN_PASSWORD );

  printStatus( "Updating $databaseConfigFile...\n" );
  $status = update_json_configuration_file( $databaseConfigFile, %db_variables );

  if ( $status == 0 )
  {
    printError( "\nInstall problem:\n" );
    printError( "    Updating of iRODS $databaseConfigFile failed.\n" );
    printError( "        ", $output );
    cleanAndExit( 1 );
  }

  chmod( 0600, $databaseConfigFile );
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
  # Create a irods_environment.json file for the user, if needed.
  printStatus( "Updating iRODS user's ~/.irods/irods_environment.json...\n" );

  # User's iRODS configuration directory doesn't exist yet.
  mkdir( $userIrodsDir, 0700 );

  # capture confiugration values from server_config.json

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


#
# @brief        Configure user accounts for a Postgres database.
#
# This function creates the user's .pgpass file so that Postgres commands
# can automatically use the right account name and password.
#
# These actions will not require a database restart.
#
# Messages are output on errors.
#
sub configureDatabaseUser()
{
  # Look for the user's .pgpass file in their home directory.
  # It may not exist yet.
  my $pgpass = File::Spec->catfile( $ENV{"HOME"}, ".pgpass" );

  # The new line for .pgpass:
  #              hostname:port:database:username:password
  my $newline = "*:$DATABASE_PORT:$DB_NAME:$DATABASE_ADMIN_NAME:$DATABASE_ADMIN_PASSWORD\n";

  printStatus( "Creating user's .pgpass...\n" );
  printToFile( $pgpass, $newline );
  chmod( 0600, $pgpass );
  return;
}


#
# @brief        Create the .odbc.ini file for the service account
#
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = success
#
sub createOdbcIni()
{
  # The user's .odbc.ini file can override the system files above,
  # which can be a problem.  Look for a section for Postgres and
  # delete it, leaving the rest of the file alone.
  printStatus( "Updating user's .odbc.ini...\n" );

  my $userODBC = File::Spec->catfile( $ENV{"HOME"}, ".odbc.ini" );

  printStatus( "    Touching... $userODBC\n" );
  open( TOUCHFILE, ">>$userODBC" );
  close( TOUCHFILE );

  # iRODS now supports a script to determine the path & lib name of the odbc driver
  my $psqlOdbcLib = "/usr/pgsql-9.3/lib/psqlodbc.so";

  open( NEWCONFIGFILE, ">$userODBC" );
  print( NEWCONFIGFILE "[postgres]\n" .
                       "Driver=$psqlOdbcLib\n" .
                       "Debug=0\n" .
                       "CommLog=0\n" .
                       "Servername=$DATABASE_HOST\n" .
                       "Database=$DB_NAME\n" .
                       "ReadOnly=no\n" .
                       "Ksqo=0\n" .
                       "Port=$DATABASE_PORT\n" );

  close( NEWCONFIGFILE );

  chmod( 0600, $userODBC );

  return 1;
}


########################################################################
#
# Finish setting up iRODS.
#

# There is a database.  We need to configure it.
my $totalSteps  = 3;
my $currentStep = 0;

# 1.  Set up the user account for the database.
++$currentStep;
printSubtitle( "\n" );
printSubtitle( "Step $currentStep of $totalSteps:  Configuring database user...\n" );
configureDatabaseUser();
createOdbcIni( );

# 2.  Configure
++$currentStep;
printSubtitle( "\n" );
printSubtitle( "Step $currentStep of $totalSteps:  Configuring iRODS server...\n" );
configureIrodsServer();

# 3.  Configure the iRODS user account.
++$currentStep;
printSubtitle( "\n" );
printSubtitle( "Step $currentStep of $totalSteps:  Configuring iRODS user and starting server...\n" );
configureIrodsUser();

# Done.
exit( 0 );
