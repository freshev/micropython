@rem = '--*-Perl-*--
@echo off
if "%OS%" == "Windows_NT" goto WinNT
IF EXIST "%~dp0perl.exe" (
"%~dp0perl.exe" -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
) ELSE IF EXIST "%~dp0..\..\bin\perl.exe" (
"%~dp0..\..\bin\perl.exe" -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
) ELSE (
perl -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
)

goto endofperl
:WinNT
IF EXIST "%~dp0perl.exe" (
"%~dp0perl.exe" -x -S %0 %*
) ELSE IF EXIST "%~dp0..\..\bin\perl.exe" (
"%~dp0..\..\bin\perl.exe" -x -S %0 %*
) ELSE (
perl -x -S %0 %*
)

if NOT "%COMSPEC%" == "%SystemRoot%\system32\cmd.exe" goto endofperl
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
if errorlevel 1 goto script_failed_so_exit_with_non_zero_val 2>nul
goto endofperl
@rem ';
#!/usr/bin/env perl
#line 29
use v5.8;
use strict;
use warnings;

use Digest ();
use Pod::Usage ();
use Getopt::Long ();

my $name = 'whirlpoolsum';
my $VERSION = '1.00';

=head1 NAME

whirlpoolsum - Print or check WHIRLPOOL checksums

=head1 DESCRIPTION

Print or check WHIRLPOOL (512-bit) checksums. With no FILE, or when
FILE is -, read standard input.

=head1 SYNOPSIS

    whirlpoolsum [OPTION] [FILE]...

=head1 OPTIONS

=over

=item -b, --binary

read files in binary mode

=item -c, --check

read WHIRLPOOL sums from FILEs and check them

=item -t, --text

read files in text mode (default)

=item -s, --status

don't output anything, status code shows success

=item -h, --help

Print a usage message listing all available options

=item -v, --version

Print the version number, then exit successfully.

=back

=head1 AUTHOR

E<AElig>var ArnfjE<ouml>rE<eth> Bjarmason <avar@cpan.org>

=head1 LICENSE

This program is free software; you can redistribute it and/or
modify it under the same terms as Perl itself.

=cut

#
# Get command line options
#

Getopt::Long::Parser->new(
	config => [ qw< bundling no_ignore_case no_require_order > ],
)->getoptions(
	'h|help' => \my $help,
	'v|version' => \my $version,

    'b|binary' => \my $binary,
    't|text' => \my $text,

    'c|check' => \my $check,
    'w|warn' => \my $warn,
    's|status' => \my $status,
) or help();

#
# Deal with --help, --version and incorrect usage
#

help( verbose => 1, exitval => 0 )
    if $help;
help( verbose => 0, exitval => 1 )
    if $binary and $text;

# Display version if requested
version( exitval => 0 )
    if $version;

#
# Set up various stuff
#

# Determine mode to read in
my $modesym = $binary ? '*' : ' ';

# read from stdin if no files are given
@ARGV = "-"
    unless @ARGV;

# rx: A line in a sum file
my $sumfmt = qr/
                      ^
                      # WHIRLPOOL sum
                      ([0-9a-f]{128})
                      # sp
                      [ ]
                      # What mode it was checked in
                      ([* ])
                      # Filename
                      (.*)
                      $
/x;

#
# Main loop
#

unless ( $check ) {
    my $err = 0;
    for my $file (@ARGV) {
        if (my $digest = sumfile($file)) {
            printf qq<%s %s%s\n>, $digest, $modesym, $file;
        } else {
            $err ||= 1;
        }
    }
    exit $err;
} else {
    my $err = 0;
    my ($num_files, $num_checksums) = (0, 0);

    # some of this is ripped from shasum(1)
    for my $sumfile (@ARGV) {
        my ($read_errs, $match_errs);
        my ($fh, $rsp);

        unless ( open my $fh, '<', $sumfile ) {
            die sprintf qq<%s: %s: %s\n>, $name, $sumfile, $!;
        } else {
            while (my $line = <$fh>) {
                # Just ignore invalid lines
                next unless $line =~ /$sumfmt/;

                my ($sum, $modesym, $file) = ($1, $2, $3);
                ($binary, $text) = map { $_ eq $modesym } ('*', ' ');

                $rsp = "$file: "; $num_files++;
                unless (my $digest = sumfile( $file )) {
                    $rsp .= "FAILED open or read\n";
                    $err ||= 1; $read_errs++;
                } else {
                    $num_checksums++;
                    if (lc $sum eq $digest) {
                        $rsp .= "OK\n";
                    } else {
                        $rsp .= "FAILED\n"; $err = 1; $match_errs++;
                    }
                }
                print $rsp
                    unless $status;
            }
            close $fh;
        }
        unless ($status) {
            warn sprintf qq<%s: WARNING: %d of %d listed files could not be read\n>,
                $name, $read_errs, $num_files
                if $read_errs;
            warn sprintf qq<%s: WARNING: %d of %d computed checksums did NOT match\n>,
                $name, $match_errs, $num_checksums
                if $match_errs;
        }

    }
	exit $err;
}

sub sumfile
{
    my ( $file ) = @_;

    my $digest;
    if ( $file eq '-' ) {
        $digest = Digest->new( 'Whirlpool' )->addfile( *STDIN );
    } else {
        eval {
            open my $fh, '<', $file;
            binmode $fh if $binary;
            $digest = Digest->new( 'Whirlpool' )->addfile( $fh );
        };
        if ($@) {
            warn sprintf qq<whirlpoolsum: %s: %s\n>, $file, $!;
            return;
        }
    }

    $digest->hexdigest;
}

sub help
{
    my %arg = @_;

    Pod::Usage::pod2usage(
        -verbose => $arg{ verbose },
        -exitval => $arg{ exitval } || 0,
    );
}

sub version
{
    my %arg = @_;
    # Spit out the same crap GNU utilities do, for the lack of something better..
    printf qq<whirlpoolsum %s\nCopyright (C) Ævar Arnfjörð Bjarmason\n>, $VERSION;
    print "This program is free software; you can redistribute it and/or\n";
    print "modify it under the same terms as Perl itself.\n";
    print "\n";
    print "Written by Ævar Arnfjörð Bjarmason <avar\@cpan.org>\n";
    exit $arg{ exitval } || 0;
}

__END__
:endofperl
