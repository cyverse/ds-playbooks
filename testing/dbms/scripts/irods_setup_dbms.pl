use warnings;

use File::Spec;

require "/tmp/utils_platform.pl";
require "/tmp/utils_print.pl";

$DATABASE_PORT           = $ARGV[0];
$DATABASE_NAME           = $ARGV[1];
$DATABASE_ADMIN_NAME     = $ARGV[2];
$DATABASE_ADMIN_PASSWORD = $ARGV[3];


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
# @brief        Send a file of SQL commands to the database.
#
# The given SQL commands are passed to the database.  The
# command to do this varies with the type of database in use.
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
sub execute_sql($$)
{
  my ($databaseName,$sqlFilename) = @_;

  $PSQL=`/tmp/find_bin_postgres.sh`;

  chomp $PSQL;
  $PSQL=$PSQL . "/psql";

  return run( "PGPASSWORD=$DATABASE_ADMIN_PASSWORD \\
               $PSQL -U $DATABASE_ADMIN_NAME -p $DATABASE_PORT $databaseName < $sqlFilename" );
}


# Create the tables.
# The iCAT SQL files issue a number of instructions to create tables and
# initialize state.
printStatus( "Creating iCAT tables...\n" );

my $status;
my $output;

my @sqlfiles = (
    "icatSysTables.sql",
    "icatSysInserts.sql",
    "icatSetupValues.sql");

my $sqlfile;
printStatus( "    Inserting iCAT tables...\n" );

my $serverSqlDir = "/tmp";

foreach $sqlfile (@sqlfiles)
{
  my $sqlPath = File::Spec->catfile( $serverSqlDir, $sqlfile );
  ($status, $output) = execute_sql( $DATABASE_NAME, $sqlPath );

  if ( $status != 0 )
  {
    # Stop if any of the SQL scripts fails.
    printError( "\nInstall problem:\n" );
    printError( "    Could not create the iCAT tables.\n" );
    printError( "        ", $output );
    cleanAndExit( 1 );
  }

  if ( $output =~ /error/i )
  {
    printError( "\nInstall problem:\n" );
    printError( "    Creation of the iCAT tables failed.\n" );
    printError( "        ", $output );
    cleanAndExit( 1 );
  }
}
