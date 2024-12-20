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
#!perl
#line 29

use strict;
use warnings;

use Mojo::Server::Morbo;
use Mojo::Util 'getopt';
use Mojolicious::Command;

getopt \@ARGV,
  'h|help'     => \my $help,
  'l|listen=s' => \my @listen,
  'm|mode=s'   => \$ENV{MOJO_MODE},
  'v|verbose'  => \$ENV{MORBO_VERBOSE},
  'w|watch=s'  => \my @watch;

die Mojolicious::Command->new->extract_usage if $help || !(my $app = shift);
my $morbo = Mojo::Server::Morbo->new;
$morbo->daemon->listen(\@listen) if @listen;
$morbo->watch(\@watch) if @watch;
$morbo->run($app);

=encoding utf8

=head1 NAME

morbo - Morbo HTTP and WebSocket development server

=head1 SYNOPSIS

  Usage: morbo [OPTIONS] [APPLICATION]

    morbo ./script/my_app
    morbo ./myapp.pl
    morbo -m production -l https://*:443 -l http://[::]:3000 ./myapp.pl
    morbo -l 'https://*:443?cert=./server.crt&key=./server.key' ./myapp.pl
    morbo -w /usr/local/lib -w public -w myapp.conf ./myapp.pl

  Options:
    -h, --help                     Show this message
    -l, --listen <location>        One or more locations you want to listen on,
                                   defaults to the value of MOJO_LISTEN or
                                   "http://*:3000"
    -m, --mode <name>              Operating mode for your application,
                                   defaults to the value of
                                   MOJO_MODE/PLACK_ENV or "development"
    -v, --verbose                  Print details about what files changed to
                                   STDOUT
    -w, --watch <directory/file>   One or more directories and files to watch
                                   for changes, defaults to the application
                                   script as well as the "lib" and "templates"
                                   directories in the current working
                                   directory

=head1 DESCRIPTION

Start L<Mojolicious> and L<Mojolicious::Lite> applications with the
L<Mojo::Server::Morbo> web server.

=head1 SEE ALSO

L<Mojolicious>, L<Mojolicious::Guides>, L<http://mojolicious.org>.

=cut

__END__
:endofperl
