#
# Perl

#
# Create the database tables and finish the iRODS installation
#
# Usage is:
#       perl irods_setup.pl [options]
#
use warnings;

# TODO uncomment this
#no warnings 'once';

use File::Spec;
use File::Copy;
use Cwd;
use Cwd "abs_path";
use Config;
use File::Basename;
use File::Path;

# =-=-=-=-=-=-=-
# detect the running environment (usually the service account user)
$scriptfullpath = "/var/lib/irods/iRODS/scripts/perl/irods_setup.pl";
$scripttoplevel = dirname(dirname(dirname(dirname($scriptfullpath))));

# =-=-=-=-=-=-=-
# for testing later...
# set flag determining if this is an automated cloud resource server
$cloudResourceInstall = 0;
if( scalar(@ARGV) == 1) {
        $cloudResourceInstall = 1;
}
# set flag to determine if this is an iCAT installation or not
$icatInstall = 0;
if( scalar(@ARGV) > 1 ) {
        $icatInstall = 1;
}

########################################################################
#
# Confirm execution from the top-level iRODS directory.
#
$IRODS_HOME = cwd( );   # Might not be actual iRODS home.  Fixed below.

my $perlScriptsDir = File::Spec->catdir( $IRODS_HOME, "scripts", "perl" );

# iRODS configuration directory
$configDir = `perl $perlScriptsDir/irods_get_config_dir.pl`;

if ( ! -d $configDir )
{
        # Configuration directory does not exist.  Perhaps this
        # script was run from the scripts or scripts/perl subdirectories.
        # Look up one directory.
        $IRODS_HOME = File::Spec->catdir( $IRODS_HOME, File::Spec->updir( ));
        $configDir  = File::Spec->catdir( $IRODS_HOME, "config" );
        if ( ! -d $configDir )
        {
                $IRODS_HOME = File::Spec->catdir( $IRODS_HOME, File::Spec->updir( ));
                $configDir  = File::Spec->catdir( $IRODS_HOME, "config" );
                if ( ! -d $configDir )
                {
                        # Nope.  Complain.
                        print( "Usage error:\n" );
                        print( "    Please run this script from the top-level directory\n" );
                        print( "    of the iRODS distribution.\n" );
                        exit( 1 );
                }
        }
}

# Make the $IRODS_HOME path absolute.
$IRODS_HOME = abs_path( $IRODS_HOME );
$configDir  = abs_path( $configDir );
$userIrodsDir = "/var/lib/irods/.irods";


########################################################################
#
# Initialize.
#

# Get the script name.  We'll use it for some print messages.
my $scriptName = $0;
my $currentPort = 0;

# Load support scripts.

push @INC, "/etc/irods";
push @INC, $configDir;
require File::Spec->catfile( $perlScriptsDir, "utils_paths.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_print.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_file.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_platform.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_config.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_prompt.pl" );
my $irodsctl = File::Spec->catfile( $perlScriptsDir, "irodsctl.pl" );

# Get the path to Perl.  We'll use it for running other Perl scripts.
my $perl = $Config{"perlpath"};
if ( !defined( $perl ) || $perl eq "" )
{
        # Not defined.  Find it.
        $perl = findCommand( "perl" );
}

# Determine the execution environment.  These values are used
# later to select different options for different OSes, or to
# print information to the log file or various configuration files.
my $thisOS     = getCurrentOS( );
my $thisUser   = getCurrentUser( );
my $thisUserID = $<;


########################################################################
#
# Check script usage.
#

# Initialize global flags.
my $noAsk = 0;
my $noHeader = 0;
my $isUpgrade = 0;
setPrintVerbose( 1 );

########################################################################
#
# Confirm that prior setup stages have probably run.
#

# Make sure the iCommand binaries exist.  They are built during 'make',
# which should already have occurred.
if ( ! -e $iadmin )
{
        printError( "Usage error:\n" );
        printError( "    The file 'iadmin' is missing.  This probably\n" );
        printError( "    means that prior setup stages have not been\n" );
        printError( "    completed yet.\n" );
        printError( "\n    Please run ./setup.\n" );
        exit( 1 );
}


########################################################################
#
# Load and validate server configuration files.
#

# binary installation
if ( -e "$scripttoplevel/packaging/binary_installation.flag" )
{
    load_server_config("/etc/irods/server_config.json");
    if ( 1 == $icatInstall )
    {
        load_database_config("/etc/irods/database_config.json");
    }
}
# run-in-place
else
{
    load_server_config("$scripttoplevel/iRODS/server/config/server_config.json");
    if ( 1 == $icatInstall )
    {
        load_database_config("$scripttoplevel/iRODS/server/config/database_config.json");
    }
}


# Make sure the home directory is set and valid.  If not, the installation
# is probably being run out of order or a prior stage failed.
if ( $IRODS_HOME eq "" || ! -e $IRODS_HOME )
{
        printError( "Usage error:\n" );
        printError( "    The IRODS_HOME setting is empty or incorrect.\n" );
        exit( 1 );
}

my $ip;

# What are the environment variables?
my $env;

$DB_NAME = $ENV{"DB_NAME"};
$IRODS_HOST = $ENV{"IRODS_HOST"};

# =-=-=-=-=-=-=-
# JMC :: if arguments are 0, we assume this is a RESOURCE installation.
if( 1 == $icatInstall )
{
        $DATABASE_TYPE           = $ARGV[0];
        $DATABASE_HOST           = $ARGV[1];
        $DATABASE_PORT           = $ARGV[2];
        $DATABASE_ADMIN_NAME     = $ARGV[3];
        $DATABASE_ADMIN_PASSWORD = $ARGV[4];
}
# =-=-=-=-=-=-=-
# TGR :: for a resource server, prompt for icat admin password
else
{
    if ( 1 == $cloudResourceInstall )
    {
        # get the password from argv
        print "\n";
        print "Reading [$IRODS_ADMIN_NAME] password from input...\n";
        print "\n";
        $IRODS_ADMIN_PASSWORD = $ARGV[0];
        chomp $IRODS_ADMIN_PASSWORD;
    }
    else {
        # call out to external shell script so live password can be hidden
        # and never shown to the user and never written to disk or visible to
        # the unix process listing (ps)
        print "\n";
        print "The following password will not be written to disk\n";
        print "or made visible to any process other than this setup script.\n";
        print "\n";
        print "  iCAT server's admin username: $IRODS_ADMIN_NAME\n";
        print "  iCAT server's admin password: ";
        if( $RUNINPLACE == 1 ) {
            $IRODS_ADMIN_PASSWORD = `$scripttoplevel/packaging/get_icat_server_password.sh`;
        } else {
          $IRODS_ADMIN_PASSWORD = `/var/lib/irods/packaging/get_icat_server_password.sh`;
        }
        print "\n";
        print "\n";
    }
}

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
# @brief        Prepare to initialize.
#
# Stop iRODS, if it is running, and start the database, if it is not.
# Test that we can communicate with the database.
#
# Output error messages and exit on problems.
#
sub prepare()
{
        ++$currentStep;
        printSubtitle( "\nStep $currentStep of $totalSteps:  Preparing to initialize...\n" );

        # Stop iRODS if it is running.
        printStatus( "Stopping iRODS server...\n" );
        $currentPort = $IRODS_PORT;
        my $status = stopIrods( );
        if ( $status == 0 )
        {
                cleanAndExit( 1 );
        }
        if ( $status == 2 )
        {
                # Already started.
                printStatus( "    Skipped.  Server already stopped.\n" );
        }

}


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

  # Create the default resource, if needed.
  printStatus( "Creating default resource...\n" );

  # Create vault path if it does not exist
  if (! -e "$RESOURCE_DIR") {
    mkpath( "$RESOURCE_DIR", {error => \my $err} );

    if (@$err) {
      printError( "\nInstall problem:\n" );
      printError( "    Cannot create default resource vault path:\n" );

      foreach $e (@$err) {
        while (($k, $v) = each (%$e)) {
          printError( "        '$k' => '$v'\n" );
        }
      }

      printError( "\n" );

      cleanAndExit( 1 );
    }
  }
}


########################################################################
#
# Support functions.
#
#       All of these functions support activities in the setup steps.
#       Some output error messages directly, but none exit the script
#       on their own.
#


#
# @brief        Get the process IDs of the iRODS servers.
#
# @return       %serverPids     associative array of arrays of
#                               process ids.  Keys are server
#                               types from %servers.
#
sub getIrodsProcessIds()
{
        my @serverPids = ();

        $parentPid=getIrodsServerPid();
        @serverPids = getFamilyProcessIds( $parentPid );

        return @serverPids;
}





#
# @brief        Start iRODS server.
#
# This function starts the iRODS server and confirms that it
# started.
#
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = started
#               2 = already started
#
sub startIrods()
{
        # See if server is already started
        my @serverPids = getIrodsProcessIds( );

        if ( $#serverPids > 0 )
        {
                return 2;
        }

        ($status,$output) = run( "$perl $irodsctl start" );

        if ( $status != 0 )
        {
                printError( "Could not start iRODS server.\n" );
                printError( "    $output\n" );
                return 0;
        }
        $output =~ s/^(.*\n){1}//; # remove duplicate first line of output
        chomp($output);
        if ( $output =~ "Validation Failed" )
        {
                printWarning( "$output\n" );
        }
        else {
                printStatus( "$output\n" );
        }
        return 1;
}


#
# @brief        Get the iRODS Server PID (Parent of the others)
#
# This function iRODS Server PID, which is the parent of the others.
#
# @return
#       The PID or "NotFound"
#
sub getIrodsServerPid()
{
        my $tmpDir="/usr/tmp";
        if (!-e $tmpDir)  {
            $tmpDir="/tmp";
        }
        $currentPort = $IRODS_PORT;
        my $processFile   = $tmpDir . "/irodsServer" . "." . $currentPort;

        my $parentPid="NotFound";
        if (open( PIDFILE, '<', $processFile )) {
            my $line;
            foreach $line (<PIDFILE>)
            {
                    my $i = index($line, " ");
                    $parentPid=substr($line,0,$i);
            }
            close( PIDFILE );
        }
        return $parentPid;
}




#
# @brief        Stop iRODS server.
#
# This function stops the iRODS server and confirms that it
# stopped.
#
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = stopped
#               2 = already stopped
#
sub stopIrods
{
        #
        # Design Notes:  The current version of this uses a file created
        #       by the irods server which records its PID and then finds
        #       the children.
        #
        # Find and kill the server process IDs
        $parentPid=getIrodsServerPid();
        my @pids = getFamilyProcessIds( $parentPid );
        my $found = 0;
        $num = @pids;
        print( "Found $num processes:\n" );
        foreach $pid (@pids)
        {
                $found = 1;
                print( "\tStopping process id $pid\n" );
                kill( 'SIGINT', $pid );
        }
        if ( ! $found )
        {
                system( "python $scripttoplevel/iRODS/scripts/python/terminate_irods_processes.py" );
                printStatus( "    There are no iRODS servers running.\n" );
                return 1;
        }

        # Repeat to catch stragglers.  This time use kill -9.
        @pids = getFamilyProcessIds( $parentPid );
        $found = 0;
        foreach $pid (@pids)
        {
                $found = 1;
                print( "\tKilling process id $pid\n" );
                kill( 9, $pid );
        }

    # no regard for PIDs
    # iRODS must kill all owned processes for packaging purposes
    system( "python $scripttoplevel/iRODS/scripts/python/terminate_irods_processes.py" );

        return 1;
}






#
# @brief        Run an iCommand and report an error, if any.
#
# This function runs the given command and collects its output.
# On an error, a message is output.
#
# @param        $command
#       the iCommand to run
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = success
#
sub runIcommand($)
{
        my ($icommand) = @_;

        my ($status,$output) = run( $icommand );
        return 1 if ( $status == 0 );   # Success


        printError( "\nInstall problem:\n" );
        printError( "    iRODS command failed:\n" );
        printError( "        Command:  $icommand\n" );
        printError( "        ", $output );
        return 0;
}





#
# @brief        Make an iRODS directory, if it doesn't already exist.
#
# This function runs the iRODS command needed to create a directory.
# It checks that the directory doesn't exist first.
#
# @param        $directory
#       the directory to create
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = success
#               2 = already exists
#
sub imkdir($)
{
        my ($directory) = @_;

        # Check if the directory already exists.
        my ($status,$output) = run( "$ils $directory" );
        if ( $status == 0 )
        {
                # The 'ls' completed fine, which means the
                # directory already exists.
                return 2;
        }

        # The 'ls' reported an error.  Make sure it is a
        # 'do not exist' error.  Otherwise it is something
        # more serious.
        if ( $output !~ /does not exist/i )
        {
                # Something more serious.
                printError( "\nInstall problem:\n" );
                printError( "    iRODS command failed:\n" );
                printError( "        Command:  $ils $directory\n" );
                printError( "        ", $output );
                return 0;
        }

        # The directory doesn't exist.  Create it.
        return runIcommand( "$iadmin mkdir -f $directory" );
}





#
# @brief        Make an iRODS admin account, if it doesn't already exist.
#
# This function runs the iRODS command needed to create a user account.
# It checks that the account doesn't exist first.
#
# @param        $username
#       the name of the user account
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = success
#               2 = already exists
#
sub imkuser($)
{
        my ($username) = @_;

        # Check if the account already exists.
        #       'iadmin lu' returns a list of accounts, one per line.
        #       Ignore leading and trailing white-space on the account name.
        my ($status,$output) = run( "$iadmin lu" );
        my $line;
        my @lines = split( "\n", $output );
        $usernameWithZone=$username . "#" . $ZONE_NAME;
        foreach $line (@lines)
        {
                if ( $line =~ /^[ \t]*$usernameWithZone[ \t]*$/ )
                {
                        # The account already exists.
                        return 2;
                }
        }

        # Create it.
        return runIcommand( "$iadmin mkuser $username rodsadmin" );
}


#
# @brief        Make an iRODS group, if it doesn't already exist.
#
# This function runs the iRODS command needed to create a user group.
# It checks that the group doesn't exist first.
#
# @param        $groupname
#       the name of the user group
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = success
#               2 = already exists
#
sub imkgroup($)
{
        my ($groupname) = @_;

        # Check if the account already exists.
        #       'iadmin lu' returns a list of accounts, one per line.
        #       Ignore leading and trailing white-space on the group name.
        my ($status,$output) = run( "$iadmin lg" );
        my $line;
        my @lines = split( "\n", $output );
        foreach $line (@lines)
        {
                if ( $line =~ /^[ \t]*$groupname[ \t]*$/ )
                {
                        # The group already exists.
                        return 2;
                }
        }

        # Create it.
        return runIcommand( "$iadmin mkgroup $groupname" );
}





#
# @brief        Give ownership permissions on a directory, if not already set.
#
# This function runs the iRODS command needed to set a directory's owner.
# It checks that the ownership wasn't already set.
#
# @param        $username
#       the name of the user account
# @param        $directory
#       the directory to change
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = success
#               2 = already set
#
sub ichown($$)
{
        my ($username,$directory) = @_;

        # The original author wanted to check the ownership of the
        # directory first, but there was not iCommand to do that at
        # the time (now, ils -A does it).  But it does no harm to just
        # set it, even if it is redundant.
        my ($status,$output) = run( "$ichmod own $username $directory" );
        return 1 if ( $status == 0 );

        # An error.
        printError( "\nInstall problem:\n" );
        printError( "    iRODS command failed:\n" );
        printError( "        Command:  $ichmod own $username $directory\n" );
        printError( "        ", $output );
        return 0;
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

        # Try to shut down iRODS server.  Ignore failure.
        stopIrods( );

        printError( "\nAbort.\n" );

        closeLog( );

        exit( $code );
}


#
# @brief        Print command-line help
#
# Print a usage message.
#
sub printUsage()
{
        my $oldVerbosity = isPrintVerbose( );
        setPrintVerbose( 1 );

        printNotice( "This script sets up the iRODS database.\n" );
        printNotice( "\n" );
        printNotice( "Usage is:\n" );
        printNotice( "    $scriptName [options]\n" );
        printNotice( "\n" );
        printNotice( "Help options:\n" );
        printNotice( "    --help      Show this help information\n" );
        printNotice( "\n" );
        printNotice( "Verbosity options:\n" );
        printNotice( "    --quiet     Suppress all messages\n" );
        printNotice( "    --verbose   Output all messages (default)\n" );
        printNotice( "    --noask     Don't ask questions, assume 'yes'\n" );
        printNotice( "\n" );

        setPrintVerbose( $oldVerbosity );
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
  my $psqlOdbcLib  = `/tmp/find_odbc_postgres.sh`;
  chomp($psqlOdbcLib);

  if ($psqlOdbcLib eq "")
  {
    printError("\nInstall Problem:\n");
    printError("    find_odbc_postgres.sh did not find odbc driver.\n");

    cleanAndExit( 1 );
  }

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


#
# @brief        Update an ODBC configuration file to include a
#               given name and value.
#
# This function edits an ODBC '.ini' file and insures that the given name
# and value are set in the given section.  The rest of the file is left
# untouched.
#
# @param        $filename
#       The name of the ODBC file to change.
# @param        $section
#       The section of the ODBC file to change.
# @param        $name
#       The name to change or add to the section.
# @param        $value
#       The value to set for that name.
# @return
#       A numeric value indicating:
#               0 = failure
#               1 = success, file changed
#               2 = success, file no changed (OK as-is)
#
sub Postgres_updateODBC()
{
        my ($filename,$section,$name,$value) = @_;

        # Edit the file in-place, thereby insuring that the
        # file maintains its permissions, ownership, hard links,
        # and other attributes.

        # Copy the original file to a temp file.
        my $tmpfile = createTempFilePath( "odbc" );
        if ( copy( $filename, $tmpfile ) != 1 )
        {
                printError( "\nInstall problem:\n" );
                printError( "    Cannot copy system's ODBC configuration file.\n" );
                printError( "        File:  $filename\n" );
                printError( "        Error: $!\n" );
                return 0;
        }
        chmod( 0600, $tmpfile );

        # Open the copy for reading and write to the original
        # file.  This will truncate the original file, then we'll
        # add lines back into it.
        open( CONFIGFILE,    "<$tmpfile" );
        open( NEWCONFIGFILE, ">$filename" );    # Truncate
        my $inSection = 0;
        my $nameFound = 0;
        my $fileChanged = 0;
        foreach $line ( <CONFIGFILE> )
        {
                if ( $line =~ /^\[[ \t]*$section/ )
                {
                        # We're in the target section.  Flag it
                        # and copy the section heading.
                        $inSection = 1;
                        print( NEWCONFIGFILE $line );
                        next;
                }

                if ( $inSection && $line =~ /^$name/ )
                {
                        # We've got a line that sets the target
                        # value to change.
                        $nameFound = 1;
                        if ( $line !~ /$name=$value/ )
                        {
                                # The value isn't the target
                                # value, so rewrite the line.
                                print( NEWCONFIGFILE "$name=$value\n" );
                                $fileChanged = 1;
                        }
                        else
                        {
                                # The value is already correct.
                                # No need to change it.  Copy
                                # the existing line.
                                print( NEWCONFIGFILE $line );
                        }
                        next;
                }

                if ( $inSection && $line =~ /^[ \t]*$/ || $line =~ /^\[/ )
                {
                        # We've hit the end of the target section
                        # by getting a blank line or a new section.
                        # If the target name hasn't been set yet,
                        # then add it now.
                        if ( ! $nameFound )
                        {
                                print( NEWCONFIGFILE "$name=$value\n" );
                                $nameFound = 1;
                                $fileChanged = 1;
                        }

                        # And output the line that ended the
                        # target section (blank or a new section).
                        $inSection = 0;
                        print( NEWCONFIGFILE $line );
                        next;
                }

                # The line is fine as-is.  Just copy it.
                print( NEWCONFIGFILE $line );
        }

        # If we hit the end of the file while still in the
        # target section, and the name hasn't been output yet,
        # then write it out.
        if ( $inSection && ! $nameFound )
        {
                print( NEWCONFIGFILE "$name=$value\n" );
                $fileChanged = 1;
        }
        close( NEWCONFIGFILE );
        close( CONFIGFILE );

        # Remove the old copy.
        unlink( $tmpfile );

        return 1 if ( $fileChanged );
        return 2;
}


#
# @brief        Send a file of SQL commands to the Oracle database.
#
# This function runs 'sqlplus' and passes it the given SQL.
#
# @param        $databaseName
#       the name of the database  (Not used); instead
#    $DATABASE_ADMIN_NAME and $DATABASE_ADMIN_PASSWORD are spliced together
#         in the form needed by sqlplus (note: should restructure the args in
#    the call tree for this, perhaps just let this and Postgres_sql set these.)
# @param        $sqlFilename
#       the name of the file
# @return
#       A 2-tuple including a numeric code indicating success
#       or failure, and the stdout/stderr of the command.
#               0 = failure
#               1 = success
#
sub Oracle_sql($$)
{
    my ($databaseName,$sqlFilename) = @_;
    my ($connectArg, $i);
    $i = index($DATABASE_ADMIN_NAME, "@");

    $dbadmin = substr($DATABASE_ADMIN_NAME, 0, $i);
    $tnsname = substr($DATABASE_ADMIN_NAME, $i+1 );
    $connectArg = $dbadmin . "/" .
              $DATABASE_ADMIN_PASSWORD . "@" .
              $tnsname;
    $sqlplus = "sqlplus";
    $exec_str = "$sqlplus '$connectArg' < $sqlFilename";
    ($code,$output) = run( "$exec_str" );

    return ($code,$output);
}


#
# @brief        Send a file of SQL commands to the MySQL database.
#
# This function runs 'isql' and passes it the given SQL.
#
# @param        $databaseName
#       the name of the database
# @param        $sqlFilename
#       the name of the file
# @return
#       A 2-tuple including a numeric code indicating success
#       or failure, and the stdout/stderr of the command.
#               0 = failure
#               1 = success
#
sub MySQL_sql($$)
{
        $findbinscript = "find_bin_".$DATABASE_TYPE.".sh";
        if ( $RUNINPLACE == 1 )
        {
                $MYSQL=`$scripttoplevel/plugins/database/packaging/$findbinscript`;
        }
        else
        {
                $MYSQL=`$scripttoplevel/packaging/$findbinscript`;
        }
        chomp $MYSQL;
        $MYSQL=$MYSQL . "/mysql";
        my ($databaseName,$sqlFilename) = @_;
        return run( "$MYSQL --user=$DATABASE_ADMIN_NAME --host=$DATABASE_HOST --port=$DATABASE_PORT --password=$DATABASE_ADMIN_PASSWORD $databaseName < $sqlFilename" );
}


########################################################################
#
# Finish setting up iRODS.
#

# There is a database.  We need to configure it.
$totalSteps  = 3;
$currentStep = 0;

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

# TODO create .irodsA

# Done.
exit( 0 );
