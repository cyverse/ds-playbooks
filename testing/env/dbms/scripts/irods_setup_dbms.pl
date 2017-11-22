use warnings;

use File::Spec;

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

  $PSQL=`/tmp/find-bin-postgres`;

  chomp $PSQL;
  $PSQL=$PSQL . "/psql";

  return run( "PGPASSWORD=$DATABASE_ADMIN_PASSWORD \\
               $PSQL -U $DATABASE_ADMIN_NAME -p $DATABASE_PORT $databaseName < $sqlFilename" );
}


########################################################################
#
# Print messages
#

#
# @brief	Print a message with a color.
#
# @param	$color
# 	the color string
# @param	@message
# 	an array of message strings
#
sub printMessage
{
	my ($color,@message) = @_;

	return if ( $#message < 0 );

	# If the first message line is just white space, then
	# presume it is an indent for the rest of the message
	# lines.  But if there's nothing more in the message
	# array, then it isn't an indent.
	my $indent = "";

	if ( $message[0] =~ /^[ \t]+$/ && $#message > 0 )
	{
		$indent .= $message[0];
		shift @message;
	}

	# Print the message line-by-line.
	my $entry;

	foreach $entry (@message)
	{
		# Add the color choice and indent at the start.
		$entry =~ s/^/$color$indent/g;

		# Add the indent for intermediate carriage returns.
		$entry =~ s/\n(?!$)/\n$indent/g;

		# Print and add the color reset to the end.
		print( $entry );
	}
}


#
# @brief        Print a status message.
#
# @param        @message
#       an array of message strings
#
sub printStatus
{
  printMessage( "", "    ", @_ );
}


#
# @brief        Run an external command.
#
# Run the command and return it's status and output.
# By routing command execution through this function,
# we can log each command, and use a portable way to
# grab the command's output.
#
# @param        $command
#       the command to run
# @return
#       a 2-tuple including a numeric exit code and the
#       stdout/stderr output of the command.
#
sub run($)
{
  my ($command) = @_;

  # Run it, capturing all output.
  my $output = `$command 2>&1`;

  return (($?>>8),$output);
}


# Create the tables.
# The iCAT SQL files issue a number of instructions to create tables and
# initialize state.
printStatus( "Creating iCAT tables...\n" );

my $status;
my $output;

my @sqlfiles = (
    "icat-sys-tables.sql",
    "icat-sys-inserts.sql",
    "icat-setup-values.sql");

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
