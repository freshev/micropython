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
#!/usr/bin/perl
#line 29
package main;

use Perl::Tidy;

my $arg_string = undef;

# give Macs a chance to provide command line parameters
if ( $^O =~ /Mac/ ) {
    $arg_string = MacPerl::Ask(
        'Please enter @ARGV (-h for help)',
        defined $ARGV[0] ? "\"$ARGV[0]\"" : ""
    );
}

Perl::Tidy::perltidy( argv => $arg_string );

__END__

=head1 NAME

perltidy - a perl script indenter and reformatter

=head1 SYNOPSIS

    perltidy [ options ] file1 file2 file3 ...
            (output goes to file1.tdy, file2.tdy, file3.tdy, ...)
    perltidy [ options ] file1 -o outfile
    perltidy [ options ] file1 -st >outfile
    perltidy [ options ] <infile >outfile

=head1 DESCRIPTION

Perltidy reads a perl script and writes an indented, reformatted script.

Many users will find enough information in L<"EXAMPLES"> to get 
started.  New users may benefit from the short tutorial 
which can be found at
http://perltidy.sourceforge.net/tutorial.html

A convenient aid to systematically defining a set of style parameters
can be found at
http://perltidy.sourceforge.net/stylekey.html

Perltidy can produce output on either of two modes, depending on the
existence of an B<-html> flag.  Without this flag, the output is passed
through a formatter.  The default formatting tries to follow the
recommendations in perlstyle(1), but it can be controlled in detail with
numerous input parameters, which are described in L<"FORMATTING
OPTIONS">.  

When the B<-html> flag is given, the output is passed through an HTML
formatter which is described in L<"HTML OPTIONS">.  

=head1 EXAMPLES

  perltidy somefile.pl

This will produce a file F<somefile.pl.tdy> containing the script reformatted
using the default options, which approximate the style suggested in 
perlstyle(1).  The source file F<somefile.pl> is unchanged.

  perltidy *.pl

Execute perltidy on all F<.pl> files in the current directory with the
default options.  The output will be in files with an appended F<.tdy>
extension.  For any file with an error, there will be a file with extension
F<.ERR>.

  perltidy -b file1.pl file2.pl

Modify F<file1.pl> and F<file2.pl> in place, and backup the originals to
F<file1.pl.bak> and F<file2.pl.bak>.  If F<file1.pl.bak> and/or F<file2.pl.bak>
already exist, they will be overwritten.

  perltidy -b -bext='/' file1.pl file2.pl

Same as the previous example except that the backup files F<file1.pl.bak> and F<file2.pl.bak> will be deleted if there are no errors.

  perltidy -gnu somefile.pl

Execute perltidy on file F<somefile.pl> with a style which approximates the
GNU Coding Standards for C programs.  The output will be F<somefile.pl.tdy>.

  perltidy -i=3 somefile.pl

Execute perltidy on file F<somefile.pl>, with 3 columns for each level of
indentation (B<-i=3>) instead of the default 4 columns.  There will not be any
tabs in the reformatted script, except for any which already exist in comments,
pod documents, quotes, and here documents.  Output will be F<somefile.pl.tdy>. 

  perltidy -i=3 -et=8 somefile.pl

Same as the previous example, except that leading whitespace will
be entabbed with one tab character per 8 spaces.

  perltidy -ce -l=72 somefile.pl

Execute perltidy on file F<somefile.pl> with all defaults except use "cuddled
elses" (B<-ce>) and a maximum line length of 72 columns (B<-l=72>) instead of
the default 80 columns.  

  perltidy -g somefile.pl

Execute perltidy on file F<somefile.pl> and save a log file F<somefile.pl.LOG>
which shows the nesting of braces, parentheses, and square brackets at
the start of every line.

  perltidy -html somefile.pl

This will produce a file F<somefile.pl.html> containing the script with
html markup.  The output file will contain an embedded style sheet in
the <HEAD> section which may be edited to change the appearance.

  perltidy -html -css=mystyle.css somefile.pl

This will produce a file F<somefile.pl.html> containing the script with
html markup.  This output file will contain a link to a separate style
sheet file F<mystyle.css>.  If the file F<mystyle.css> does not exist,
it will be created.  If it exists, it will not be overwritten.

  perltidy -html -pre somefile.pl

Write an html snippet with only the PRE section to F<somefile.pl.html>.
This is useful when code snippets are being formatted for inclusion in a
larger web page.  No style sheet will be written in this case.  

  perltidy -html -ss >mystyle.css

Write a style sheet to F<mystyle.css> and exit.

  perltidy -html -frm mymodule.pm

Write html with a frame holding a table of contents and the source code.  The
output files will be F<mymodule.pm.html> (the frame), F<mymodule.pm.toc.html>
(the table of contents), and F<mymodule.pm.src.html> (the source code).

=head1 OPTIONS - OVERVIEW

The entire command line is scanned for options, and they are processed
before any files are processed.  As a result, it does not matter
whether flags are before or after any filenames.  However, the relative
order of parameters is important, with later parameters overriding the
values of earlier parameters.  

For each parameter, there is a long name and a short name.  The short
names are convenient for keyboard input, while the long names are
self-documenting and therefore useful in scripts.  It is customary to
use two leading dashes for long names, but one may be used.

Most parameters which serve as on/off flags can be negated with a
leading "n" (for the short name) or a leading "no" or "no-" (for the
long name).  For example, the flag to outdent long quotes is B<-olq>
or B<--outdent-long-quotes>.  The flag to skip this is B<-nolq>
or B<--nooutdent-long-quotes> or B<--no-outdent-long-quotes>.

Options may not be bundled together.  In other words, options B<-q> and
B<-g> may NOT be entered as B<-qg>.

Option names may be terminated early as long as they are uniquely identified.
For example, instead of B<--dump-token-types>, it would be sufficient to enter
B<--dump-tok>, or even B<--dump-t>, to uniquely identify this command.

=head2 I/O control

The following parameters concern the files which are read and written.

=over 4

=item B<-h>,    B<--help> 

Show summary of usage and exit.

=item	B<-o>=filename,    B<--outfile>=filename  

Name of the output file (only if a single input file is being
processed).  If no output file is specified, and output is not
redirected to the standard output, the output will go to F<filename.tdy>.

=item	B<-st>,    B<--standard-output>

Perltidy must be able to operate on an arbitrarily large number of files
in a single run, with each output being directed to a different output
file.  Obviously this would conflict with outputting to the single
standard output device, so a special flag, B<-st>, is required to
request outputting to the standard output.  For example,

  perltidy somefile.pl -st >somefile.new.pl

This option may only be used if there is just a single input file.  
The default is B<-nst> or B<--nostandard-output>.

=item	B<-se>,    B<--standard-error-output>

If perltidy detects an error when processing file F<somefile.pl>, its
default behavior is to write error messages to file F<somefile.pl.ERR>.
Use B<-se> to cause all error messages to be sent to the standard error
output stream instead.  This directive may be negated with B<-nse>.
Thus, you may place B<-se> in a F<.perltidyrc> and override it when
desired with B<-nse> on the command line.

=item	B<-oext>=ext,    B<--output-file-extension>=ext  

Change the extension of the output file to be F<ext> instead of the
default F<tdy> (or F<html> in case the -B<-html> option is used).
See L<Specifying File Extensions>.

=item	B<-opath>=path,    B<--output-path>=path  

When perltidy creates a filename for an output file, by default it merely
appends an extension to the path and basename of the input file.  This
parameter causes the path to be changed to F<path> instead.

The path should end in a valid path separator character, but perltidy will try
to add one if it is missing.

For example
 
 perltidy somefile.pl -opath=/tmp/

will produce F</tmp/somefile.pl.tdy>.  Otherwise, F<somefile.pl.tdy> will
appear in whatever directory contains F<somefile.pl>.

If the path contains spaces, it should be placed in quotes.

This parameter will be ignored if output is being directed to standard output,
or if it is being specified explicitly with the B<-o=s> parameter.

=item	B<-b>,    B<--backup-and-modify-in-place>

Modify the input file or files in-place and save the original with the
extension F<.bak>.  Any existing F<.bak> file will be deleted.  See next
item for changing the default backup extension, and for eliminating the
backup file altogether.  

A B<-b> flag will be ignored if input is from standard input or goes to
standard output, or if the B<-html> flag is set.  

In particular, if you want to use both the B<-b> flag and the B<-pbp>
(--perl-best-practices) flag, then you must put a B<-nst> flag after the
B<-pbp> flag because it contains a B<-st> flag as one of its components,
which means that output will go to the standard output stream.

=item	B<-bext>=ext,    B<--backup-file-extension>=ext  

This parameter serves two purposes: (1) to change the extension of the backup
file to be something other than the default F<.bak>, and (2) to indicate
that no backup file should be saved.

To change the default extension to something other than F<.bak> see
L<Specifying File Extensions>.

A backup file of the source is always written, but you can request that it
be deleted at the end of processing if there were no errors.  This is risky
unless the source code is being maintained with a source code control
system.  

To indicate that the backup should be deleted include one forward slash,
B</>, in the extension.  If any text remains after the slash is removed
it will be used to define the backup file extension (which is always
created and only deleted if there were no errors).

Here are some examples:

  Parameter           Extension          Backup File Treatment
  <-bext=bak>         F<.bak>            Keep (same as the default behavior)
  <-bext='/'>         F<.bak>            Delete if no errors
  <-bext='/backup'>   F<.backup>         Delete if no errors
  <-bext='original/'> F<.original>       Delete if no errors

=item B<-w>,    B<--warning-output>             

Setting B<-w> causes any non-critical warning
messages to be reported as errors.  These include messages
about possible pod problems, possibly bad starting indentation level,
and cautions about indirect object usage.  The default, B<-nw> or
B<--nowarning-output>, is not to include these warnings.

=item B<-q>,    B<--quiet>             

Deactivate error messages and syntax checking (for running under
an editor). 

For example, if you use a vi-style editor, such as vim, you may execute
perltidy as a filter from within the editor using something like

 :n1,n2!perltidy -q

where C<n1,n2> represents the selected text.  Without the B<-q> flag,
any error message may mess up your screen, so be prepared to use your
"undo" key.

=item B<-log>,    B<--logfile>           

Save the F<.LOG> file, which has many useful diagnostics.  Perltidy always
creates a F<.LOG> file, but by default it is deleted unless a program bug is
suspected.  Setting the B<-log> flag forces the log file to be saved.

=item B<-g=n>, B<--logfile-gap=n>

Set maximum interval between input code lines in the logfile.  This purpose of
this flag is to assist in debugging nesting errors.  The value of C<n> is
optional.  If you set the flag B<-g> without the value of C<n>, it will be
taken to be 1, meaning that every line will be written to the log file.  This
can be helpful if you are looking for a brace, paren, or bracket nesting error. 

Setting B<-g> also causes the logfile to be saved, so it is not necessary to
also include B<-log>. 

If no B<-g> flag is given, a value of 50 will be used, meaning that at least
every 50th line will be recorded in the logfile.  This helps prevent
excessively long log files.  

Setting a negative value of C<n> is the same as not setting B<-g> at all.

=item B<-npro>  B<--noprofile>    

Ignore any F<.perltidyrc> command file.  Normally, perltidy looks first in
your current directory for a F<.perltidyrc> file of parameters.  (The format
is described below).  If it finds one, it applies those options to the
initial default values, and then it applies any that have been defined
on the command line.  If no F<.perltidyrc> file is found, it looks for one
in your home directory.

If you set the B<-npro> flag, perltidy will not look for this file.

=item B<-pro=filename> or  B<--profile=filename>    

To simplify testing and switching .perltidyrc files, this command may be
used to specify a configuration file which will override the default
name of .perltidyrc.  There must not be a space on either side of the
'=' sign.  For example, the line

   perltidy -pro=testcfg

would cause file F<testcfg> to be used instead of the 
default F<.perltidyrc>.

A pathname begins with three dots, e.g. ".../.perltidyrc", indicates that
the file should be searched for starting in the current directory and
working upwards. This makes it easier to have multiple projects each with
their own .perltidyrc in their root directories.

=item B<-opt>,   B<--show-options>      

Write a list of all options used to the F<.LOG> file.  
Please see B<--dump-options> for a simpler way to do this.

=item B<-f>,   B<--force-read-binary>      

Force perltidy to process binary files.  To avoid producing excessive
error messages, perltidy skips files identified by the system as non-text.
However, valid perl scripts containing binary data may sometimes be identified
as non-text, and this flag forces perltidy to process them.

=back

=head1 FORMATTING OPTIONS

=head2 Basic Options

=over 4

=item B<--notidy>

This flag disables all formatting and causes the input to be copied unchanged
to the output except for possible changes in line ending characters and any
pre- and post-filters.  This can be useful in conjunction with a hierarchical
set of F<.perltidyrc> files to avoid unwanted code tidying.  See also
L<Skipping Selected Sections of Code> for a way to avoid tidying specific
sections of code.

=item B<-i=n>,  B<--indent-columns=n>  

Use n columns per indentation level (default n=4).

=item B<-l=n>, B<--maximum-line-length=n>

The default maximum line length is n=80 characters.  Perltidy will try
to find line break points to keep lines below this length. However, long
quotes and side comments may cause lines to exceed this length. 
Setting B<-l=0> is equivalent to setting B<-l=(a large number)>. 

=item B<-vmll>, B<--variable-maximum-line-length>

A problem arises using a fixed maximum line length with very deeply nested code
and data structures because eventually the amount of leading whitespace used
for indicating indentation takes up most or all of the available line width,
leaving little or no space for the actual code or data.  One solution is to use
a vary long line length.  Another solution is to use the B<-vmll> flag, which
basically tells perltidy to ignore leading whitespace when measuring the line
length.  

To be precise, when the B<-vmll> parameter is set, the maximum line length of a
line of code will be M+L*I, where

      M is the value of --maximum-line-length=M (-l=M), default 80,
      I is the value of --indent-columns=I (-i=I), default 4,
      L is the indentation level of the line of code

When this flag is set, the choice of breakpoints for a block of code should be
essentially independent of its nesting depth.  However, the absolute line
lengths, including leading whitespace, can still be arbitrarily large.  This
problem can be avoided by including the next parameter.  

The default is not to do this (B<-nvmll>).

=item B<-wc=n>, B<--whitespace-cycle=n>

This flag also addresses problems with very deeply nested code and data
structures.  When the nesting depth exceeds the value B<n> the leading
whitespace will be reduced and start at a depth of 1 again.  The result is that
blocks of code will shift back to the left rather than moving arbitrarily far
to the right.  This occurs cyclically to any depth.  

For example if one level of indentation equals 4 spaces (B<-i=4>, the default),
and one uses B<-wc=15>, then if the leading whitespace on a line exceeds about
4*15=60 spaces it will be reduced back to 4*1=4 spaces and continue increasing
from there.  If the whitespace never exceeds this limit the formatting remains
unchanged.

The combination of B<-vmll> and B<-wc=n> provides a solution to the problem of
displaying arbitrarily deep data structures and code in a finite window,
although B<-wc=n> may of course be used without B<-vmll>.

The default is not to use this, which can also be indicated using B<-wc=0>.

=item tabs

Using tab characters will almost certainly lead to future portability
and maintenance problems, so the default and recommendation is not to
use them.  For those who prefer tabs, however, there are two different
options.  

Except for possibly introducing tab indentation characters, as outlined
below, perltidy does not introduce any tab characters into your file,
and it removes any tabs from the code (unless requested not to do so
with B<-fws>).  If you have any tabs in your comments, quotes, or
here-documents, they will remain.

=over 4

=item B<-et=n>,   B<--entab-leading-whitespace>

This flag causes each B<n> initial space characters to be replaced by
one tab character.  Note that the integer B<n> is completely independent
of the integer specified for indentation parameter, B<-i=n>.

=item B<-t>,   B<--tabs>

This flag causes one leading tab character to be inserted for each level
of indentation.  Certain other features are incompatible with this
option, and if these options are also given, then a warning message will
be issued and this flag will be unset.  One example is the B<-lp>
option.

=item B<-dt=n>,   B<--default-tabsize=n>

If the first line of code passed to perltidy contains leading tabs but no
tab scheme is specified for the output stream then perltidy must guess how many
spaces correspond to each leading tab.  This number of spaces B<n>
corresponding to each leading tab of the input stream may be specified with
B<-dt=n>.  The default is B<n=8>.  

This flag has no effect if a tab scheme is specified for the output stream,
because then the input stream is assumed to use the same tab scheme and
indentation spaces as for the output stream (any other assumption would lead to
unstable editing).

=back

=item B<-syn>,   B<--check-syntax>      

This flag causes perltidy to run C<perl -c -T> to check syntax of input
and output.  (To change the flags passed to perl, see the next
item, B<-pscf>).  The results are written to the F<.LOG> file, which
will be saved if an error is detected in the output script.  The output
script is not checked if the input script has a syntax error.  Perltidy
does its own checking, but this option employs perl to get a "second
opinion".

If perl reports errors in the input file, they will not be reported in
the error output unless the B<--warning-output> flag is given. 

The default is B<NOT> to do this type of syntax checking (although
perltidy will still do as much self-checking as possible).  The reason
is that it causes all code in BEGIN blocks to be executed, for all
modules being used, and this opens the door to security issues and
infinite loops when running perltidy.

=item B<-pscf=s>, B<-perl-syntax-check-flags=s>

When perl is invoked to check syntax, the normal flags are C<-c -T>.  In
addition, if the B<-x> flag is given to perltidy, then perl will also be
passed a B<-x> flag.  It should not normally be necessary to change
these flags, but it can be done with the B<-pscf=s> flag.  For example,
if the taint flag, C<-T>, is not wanted, the flag could be set to be just
B<-pscf=-c>.  

Perltidy will pass your string to perl with the exception that it will
add a B<-c> and B<-x> if appropriate.  The F<.LOG> file will show
exactly what flags were passed to perl.

=item B<-xs>,   B<--extended-syntax>      

A problem with formatting Perl code is that some modules can introduce new
syntax.  This flag allows perltidy to handle certain common extensions
to the standard syntax without complaint.  

For example, without this flag a structure such as the following would generate
a syntax error and the braces would not be balanced:

    method deposit( Num $amount) {
        $self->balance( $self->balance + $amount );
    }

This flag is enabled by default but it can be deactivated with B<-nxs>.
Probably the only reason to deactivate this flag is to generate more diagnostic
messages when debugging a script.


=item B<-io>,   B<--indent-only>       

This flag is used to deactivate all whitespace and line break changes
within non-blank lines of code.
When it is in effect, the only change to the script will be
to the indentation and to the number of blank lines.
And any flags controlling whitespace and newlines will be ignored.  You
might want to use this if you are perfectly happy with your whitespace
and line breaks, and merely want perltidy to handle the indentation.
(This also speeds up perltidy by well over a factor of two, so it might be
useful when perltidy is merely being used to help find a brace error in
a large script).

Setting this flag is equivalent to setting B<--freeze-newlines> and
B<--freeze-whitespace>.  

If you also want to keep your existing blank lines exactly
as they are, you can add B<--freeze-blank-lines>. 

With this option perltidy is still free to modify the indenting (and
outdenting) of code and comments as it normally would.  If you also want to
prevent long comment lines from being outdented, you can add either B<-noll> or
B<-l=0>.

Setting this flag will prevent perltidy from doing any special operations on
closing side comments.  You may still delete all side comments however when
this flag is in effect.


=item B<-enc=s>,  B<--character-encoding=s>

where B<s>=B<none> or B<utf8>.  This flag tells perltidy the character encoding
of both the input and output character streams.  The value B<utf8> causes the
stream to be read and written as UTF-8.  The value B<none> causes the stream to
be processed without special encoding assumptions.  At present there is no
automatic detection of character encoding (even if there is a C<'use utf8'>
statement in your code) so this flag must be set for streams encoded in UTF-8.
Incorrectly setting this parameter can cause data corruption, so please
carefully check the output.

The default is B<none>.  

The abbreviations B<-utf8> or B<-UTF8> are equivalent to B<-enc=utf8>.
So to process a file named B<file.pl> which is encoded in UTF-8 you can use:

   perltidy -utf8 file.pl

=item B<-ole=s>,  B<--output-line-ending=s>

where s=C<win>, C<dos>, C<unix>, or C<mac>.  This flag tells perltidy
to output line endings for a specific system.  Normally,
perltidy writes files with the line separator character of the host
system.  The C<win> and C<dos> flags have an identical result.

=item B<-ple>,  B<--preserve-line-endings>

This flag tells perltidy to write its output files with the same line
endings as the input file, if possible.  It should work for
B<dos>, B<unix>, and B<mac> line endings.  It will only work if perltidy
input comes from a filename (rather than stdin, for example).  If
perltidy has trouble determining the input file line ending, it will
revert to the default behavior of using the line ending of the host system.

=item B<-it=n>,   B<--iterations=n>

This flag causes perltidy to do B<n> complete iterations.  The reason for this
flag is that code beautification is an iterative process and in some
cases the output from perltidy can be different if it is applied a second time.
For most purposes the default of B<n=1> should be satisfactory.  However B<n=2>
can be useful when a major style change is being made, or when code is being
beautified on check-in to a source code control system.  It has been found to
be extremely rare for the output to change after 2 iterations.  If a value
B<n> is greater than 2 is input then a convergence test will be used to stop
the iterations as soon as possible, almost always after 2 iterations.  See
the next item for a simplified iteration control.

This flag has no effect when perltidy is used to generate html.

=item B<-conv>,   B<--converge>

This flag is equivalent to B<-it=4> and is included to simplify iteration
control.  For all practical purposes one either does or does not want to be
sure that the output is converged, and there is no penalty to using a large
iteration limit since perltidy will check for convergence and stop iterating as
soon as possible.  The default is B<-nconv> (no convergence check).  Using
B<-conv> will approximately double run time since normally one extra iteration
is required to verify convergence.

=back

=head2 Code Indentation Control

=over 4

=item B<-ci=n>, B<--continuation-indentation=n>

Continuation indentation is extra indentation spaces applied when
a long line is broken.  The default is n=2, illustrated here:

 my $level =   # -ci=2      
   ( $max_index_to_go >= 0 ) ? $levels_to_go[0] : $last_output_level;

The same example, with n=0, is a little harder to read:

 my $level =   # -ci=0    
 ( $max_index_to_go >= 0 ) ? $levels_to_go[0] : $last_output_level;

The value given to B<-ci> is also used by some commands when a small
space is required.  Examples are commands for outdenting labels,
B<-ola>, and control keywords, B<-okw>.  

When default values are not used, it is suggested that the value B<n>
given with B<-ci=n> be no more than about one-half of the number of
spaces assigned to a full indentation level on the B<-i=n> command.

=item B<-sil=n> B<--starting-indentation-level=n>   

By default, perltidy examines the input file and tries to determine the
starting indentation level.  While it is often zero, it may not be
zero for a code snippet being sent from an editing session.  

To guess the starting indentation level perltidy simply assumes that
indentation scheme used to create the code snippet is the same as is being used
for the current perltidy process.  This is the only sensible guess that can be
made.  It should be correct if this is true, but otherwise it probably won't.
For example, if the input script was written with -i=2 and the current peltidy
flags have -i=4, the wrong initial indentation will be guessed for a code
snippet which has non-zero initial indentation. Likewise, if an entabbing
scheme is used in the input script and not in the current process then the
guessed indentation will be wrong.

If the default method does not work correctly, or you want to change the
starting level, use B<-sil=n>, to force the starting level to be n.

=item List indentation using B<-lp>, B<--line-up-parentheses>

By default, perltidy indents lists with 4 spaces, or whatever value
is specified with B<-i=n>.  Here is a small list formatted in this way:

    # perltidy (default)
    @month_of_year = (
        'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
        'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'
    );

Use the B<-lp> flag to add extra indentation to cause the data to begin
past the opening parentheses of a sub call or list, or opening square
bracket of an anonymous array, or opening curly brace of an anonymous
hash.  With this option, the above list would become:

    # perltidy -lp
    @month_of_year = (
                       'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
                       'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'
    );

If the available line length (see B<-l=n> ) does not permit this much 
space, perltidy will use less.   For alternate placement of the
closing paren, see the next section.

This option has no effect on code BLOCKS, such as if/then/else blocks,
which always use whatever is specified with B<-i=n>.  Also, the
existence of line breaks and/or block comments between the opening and
closing parens may cause perltidy to temporarily revert to its default
method.

Note: The B<-lp> option may not be used together with the B<-t> tabs option.
It may, however, be used with the B<-et=n> tab method.

In addition, any parameter which significantly restricts the ability of
perltidy to choose newlines will conflict with B<-lp> and will cause
B<-lp> to be deactivated.  These include B<-io>, B<-fnl>, B<-nanl>, and
B<-ndnl>.  The reason is that the B<-lp> indentation style can require
the careful coordination of an arbitrary number of break points in
hierarchical lists, and these flags may prevent that.

=item B<-cti=n>, B<--closing-token-indentation>

The B<-cti=n> flag controls the indentation of a line beginning with 
a C<)>, C<]>, or a non-block C<}>.  Such a line receives:

 -cti = 0 no extra indentation (default)
 -cti = 1 extra indentation such that the closing token
        aligns with its opening token.
 -cti = 2 one extra indentation level if the line looks like:
        );  or  ];  or  };
 -cti = 3 one extra indentation level always

The flags B<-cti=1> and B<-cti=2> work well with the B<-lp> flag (previous
section).
    
    # perltidy -lp -cti=1
    @month_of_year = (
                       'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
                       'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'
                     );

    # perltidy -lp -cti=2
    @month_of_year = (
                       'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
                       'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'
                       );

These flags are merely hints to the formatter and they may not always be
followed.  In particular, if -lp is not being used, the indentation for
B<cti=1> is constrained to be no more than one indentation level.

If desired, this control can be applied independently to each of the
closing container token types.  In fact, B<-cti=n> is merely an
abbreviation for B<-cpi=n -csbi=n -cbi=n>, where:  
B<-cpi> or B<--closing-paren-indentation> controls B<)>'s,
B<-csbi> or B<--closing-square-bracket-indentation> controls B<]>'s, 
B<-cbi> or B<--closing-brace-indentation> controls non-block B<}>'s. 

=item B<-icp>, B<--indent-closing-paren>

The B<-icp> flag is equivalent to
B<-cti=2>, described in the previous section.  The B<-nicp> flag is
equivalent B<-cti=0>.  They are included for backwards compatibility.

=item B<-icb>, B<--indent-closing-brace>

The B<-icb> option gives one extra level of indentation to a brace which
terminates a code block .  For example,

        if ($task) {
            yyy();
            }    # -icb
        else {
            zzz();
            }

The default is not to do this, indicated by B<-nicb>.

=item B<-olq>, B<--outdent-long-quotes>

When B<-olq> is set, lines which is a quoted string longer than the
value B<maximum-line-length> will have their indentation removed to make
them more readable.  This is the default.  To prevent such out-denting,
use B<-nolq> or B<--nooutdent-long-lines>.

=item B<-oll>, B<--outdent-long-lines>

This command is equivalent to B<--outdent-long-quotes> and
B<--outdent-long-comments>, and it is included for compatibility with previous
versions of perltidy.  The negation of this also works, B<-noll> or
B<--nooutdent-long-lines>, and is equivalent to setting B<-nolq> and B<-nolc>.

=item Outdenting Labels: B<-ola>,  B<--outdent-labels>

This command will cause labels to be outdented by 2 spaces (or whatever B<-ci>
has been set to), if possible.  This is the default.  For example:

        my $i;
      LOOP: while ( $i = <FOTOS> ) {
            chomp($i);
            next unless $i;
            fixit($i);
        }

Use B<-nola> to not outdent labels. 

=item Outdenting Keywords

=over 4

=item B<-okw>,  B<--outdent-keywords>

The command B<-okw> will cause certain leading control keywords to
be outdented by 2 spaces (or whatever B<-ci> has been set to), if
possible.  By default, these keywords are C<redo>, C<next>, C<last>,
C<goto>, and C<return>.  The intention is to make these control keywords
easier to see.  To change this list of keywords being outdented, see
the next section.

For example, using C<perltidy -okw> on the previous example gives:

        my $i;
      LOOP: while ( $i = <FOTOS> ) {
            chomp($i);
          next unless $i;
            fixit($i);
        }

The default is not to do this.  

=item Specifying Outdented Keywords: B<-okwl=string>,  B<--outdent-keyword-list=string>

This command can be used to change the keywords which are outdented with
the B<-okw> command.  The parameter B<string> is a required list of perl
keywords, which should be placed in quotes if there are more than one.
By itself, it does not cause any outdenting to occur, so the B<-okw>
command is still required.

For example, the commands C<-okwl="next last redo goto" -okw> will cause
those four keywords to be outdented.  It is probably simplest to place
any B<-okwl> command in a F<.perltidyrc> file.

=back

=back

=head2 Whitespace Control

Whitespace refers to the blank space between variables, operators,
and other code tokens.

=over 4

=item B<-fws>,  B<--freeze-whitespace>

This flag causes your original whitespace to remain unchanged, and
causes the rest of the whitespace commands in this section, the
Code Indentation section, and
the Comment Control section to be ignored.

=item Tightness of curly braces, parentheses, and square brackets.

Here the term "tightness" will mean the closeness with which
pairs of enclosing tokens, such as parentheses, contain the quantities
within.  A numerical value of 0, 1, or 2 defines the tightness, with
0 being least tight and 2 being most tight.  Spaces within containers
are always symmetric, so if there is a space after a C<(> then there
will be a space before the corresponding C<)>.

The B<-pt=n> or B<--paren-tightness=n> parameter controls the space within
parens.  The example below shows the effect of the three possible
values, 0, 1, and 2:

 if ( ( my $len_tab = length( $tabstr ) ) > 0 ) {  # -pt=0
 if ( ( my $len_tab = length($tabstr) ) > 0 ) {    # -pt=1 (default)
 if ((my $len_tab = length($tabstr)) > 0) {        # -pt=2

When n is 0, there is always a space to the right of a '(' and to the left
of a ')'.  For n=2 there is never a space.  For n=1, the default, there
is a space unless the quantity within the parens is a single token, such
as an identifier or quoted string.  

Likewise, the parameter B<-sbt=n> or B<--square-bracket-tightness=n>
controls the space within square brackets, as illustrated below.

 $width = $col[ $j + $k ] - $col[ $j ];  # -sbt=0
 $width = $col[ $j + $k ] - $col[$j];    # -sbt=1 (default)
 $width = $col[$j + $k] - $col[$j];      # -sbt=2 

Curly braces which do not contain code blocks are controlled by
the parameter B<-bt=n> or B<--brace-tightness=n>. 

 $obj->{ $parsed_sql->{ 'table' }[0] };    # -bt=0
 $obj->{ $parsed_sql->{'table'}[0] };      # -bt=1 (default)
 $obj->{$parsed_sql->{'table'}[0]};        # -bt=2

And finally, curly braces which contain blocks of code are controlled by the
parameter B<-bbt=n> or B<--block-brace-tightness=n> as illustrated in the
example below.   

 %bf = map { $_ => -M $_ } grep { /\.deb$/ } dirents '.'; # -bbt=0 (default)
 %bf = map { $_ => -M $_ } grep {/\.deb$/} dirents '.';   # -bbt=1
 %bf = map {$_ => -M $_} grep {/\.deb$/} dirents '.';     # -bbt=2

To simplify input in the case that all of the tightness flags have the same
value <n>, the parameter <-act=n> or B<--all-containers-tightness=n> is an
abbreviation for the combination <-pt=n -sbt=n -bt=n -bbt=n>.


=item B<-tso>,   B<--tight-secret-operators>

The flag B<-tso> causes certain perl token sequences (secret operators)
which might be considered to be a single operator to be formatted "tightly"
(without spaces).  The operators currently modified by this flag are: 

     0+  +0  ()x!! ~~<>  ,=>   =( )=  

For example the sequence B<0 +>,  which converts a string to a number,
would be formatted without a space: B<0+> when the B<-tso> flag is set.  This
flag is off by default.

=item B<-sts>,   B<--space-terminal-semicolon>

Some programmers prefer a space before all terminal semicolons.  The
default is for no such space, and is indicated with B<-nsts> or
B<--nospace-terminal-semicolon>.

	$i = 1 ;     #  -sts
	$i = 1;      #  -nsts   (default)

=item B<-sfs>,   B<--space-for-semicolon>

Semicolons within B<for> loops may sometimes be hard to see,
particularly when commas are also present.  This option places spaces on
both sides of these special semicolons, and is the default.  Use
B<-nsfs> or B<--nospace-for-semicolon> to deactivate it.

 for ( @a = @$ap, $u = shift @a ; @a ; $u = $v ) {  # -sfs (default)
 for ( @a = @$ap, $u = shift @a; @a; $u = $v ) {    # -nsfs

=item B<-asc>,  B<--add-semicolons>

Setting B<-asc> allows perltidy to add any missing optional semicolon at the end 
of a line which is followed by a closing curly brace on the next line.  This
is the default, and may be deactivated with B<-nasc> or B<--noadd-semicolons>.

=item B<-dsm>,  B<--delete-semicolons>

Setting B<-dsm> allows perltidy to delete extra semicolons which are
simply empty statements.  This is the default, and may be deactivated
with B<-ndsm> or B<--nodelete-semicolons>.  (Such semicolons are not
deleted, however, if they would promote a side comment to a block
comment).

=item B<-aws>,  B<--add-whitespace>

Setting this option allows perltidy to add certain whitespace improve
code readability.  This is the default. If you do not want any
whitespace added, but are willing to have some whitespace deleted, use
B<-naws>.  (Use B<-fws> to leave whitespace completely unchanged).

=item B<-dws>,  B<--delete-old-whitespace>

Setting this option allows perltidy to remove some old whitespace
between characters, if necessary.  This is the default.  If you
do not want any old whitespace removed, use B<-ndws> or
B<--nodelete-old-whitespace>.

=item Detailed whitespace controls around tokens

For those who want more detailed control over the whitespace around
tokens, there are four parameters which can directly modify the default
whitespace rules built into perltidy for any token.  They are:

B<-wls=s> or B<--want-left-space=s>,

B<-nwls=s> or B<--nowant-left-space=s>,

B<-wrs=s> or B<--want-right-space=s>,

B<-nwrs=s> or B<--nowant-right-space=s>.

These parameters are each followed by a quoted string, B<s>, containing a
list of token types.  No more than one of each of these parameters
should be specified, because repeating a command-line parameter
always overwrites the previous one before perltidy ever sees it.

To illustrate how these are used, suppose it is desired that there be no
space on either side of the token types B<= + - / *>.  The following two
parameters would specify this desire:

  -nwls="= + - / *"    -nwrs="= + - / *"

(Note that the token types are in quotes, and that they are separated by
spaces).  With these modified whitespace rules, the following line of math:

  $root = -$b + sqrt( $b * $b - 4. * $a * $c ) / ( 2. * $a );

becomes this:

  $root=-$b+sqrt( $b*$b-4.*$a*$c )/( 2.*$a );

These parameters should be considered to be hints to perltidy rather
than fixed rules, because perltidy must try to resolve conflicts that
arise between them and all of the other rules that it uses.  One
conflict that can arise is if, between two tokens, the left token wants
a space and the right one doesn't.  In this case, the token not wanting
a space takes priority.  

It is necessary to have a list of all token types in order to create
this type of input.  Such a list can be obtained by the command
B<--dump-token-types>.  Also try the B<-D> flag on a short snippet of code
and look at the .DEBUG file to see the tokenization. 

B<WARNING> Be sure to put these tokens in quotes to avoid having them
misinterpreted by your command shell.

=item Space between specific keywords and opening paren

When an opening paren follows a Perl keyword, no space is introduced after the
keyword, unless it is (by default) one of these:

   my local our and or eq ne if else elsif until unless 
   while for foreach return switch case given when

These defaults can be modified with two commands:

B<-sak=s>  or B<--space-after-keyword=s>  adds keywords.

B<-nsak=s>  or B<--nospace-after-keyword=s>  removes keywords.

where B<s> is a list of keywords (in quotes if necessary).  For example, 

  my ( $a, $b, $c ) = @_;    # default
  my( $a, $b, $c ) = @_;     # -nsak="my local our"

The abbreviation B<-nsak='*'> is equivalent to including all of the
keywords in the above list.

When both B<-nsak=s> and B<-sak=s> commands are included, the B<-nsak=s>
command is executed first.  For example, to have space after only the
keywords (my, local, our) you could use B<-nsak="*" -sak="my local our">.

To put a space after all keywords, see the next item.

=item Space between all keywords and opening parens

When an opening paren follows a function or keyword, no space is introduced
after the keyword except for the keywords noted in the previous item.  To
always put a space between a function or keyword and its opening paren,
use the command:

B<-skp>  or B<--space-keyword-paren>

You will probably also want to use the flag B<-sfp> (next item) too.

=item Space between all function names and opening parens

When an opening paren follows a function the default is not to introduce
a space.  To cause a space to be introduced use:

B<-sfp>  or B<--space-function-paren>

  myfunc( $a, $b, $c );    # default 
  myfunc ( $a, $b, $c );   # -sfp

You will probably also want to use the flag B<-skp> (previous item) too.

=item Trimming whitespace around C<qw> quotes

B<-tqw> or B<--trim-qw> provide the default behavior of trimming
spaces around multi-line C<qw> quotes and indenting them appropriately.

B<-ntqw> or B<--notrim-qw> cause leading and trailing whitespace around
multi-line C<qw> quotes to be left unchanged.  This option will not
normally be necessary, but was added for testing purposes, because in
some versions of perl, trimming C<qw> quotes changes the syntax tree.

=item Trimming trailing whitespace from lines of POD

B<-trp> or B<--trim-pod> will remove trailing whitespace from lines of POD.
The default is not to do this.

=back

=head2 Comment Controls

Perltidy has a number of ways to control the appearance of both block comments
and side comments.  The term B<block comment> here refers to a full-line
comment, whereas B<side comment> will refer to a comment which appears on a
line to the right of some code.

=over 4

=item B<-ibc>,  B<--indent-block-comments>

Block comments normally look best when they are indented to the same
level as the code which follows them.  This is the default behavior, but
you may use B<-nibc> to keep block comments left-justified.  Here is an
example:

             # this comment is indented      (-ibc, default)
	     if ($task) { yyy(); }

The alternative is B<-nibc>:

 # this comment is not indented              (-nibc)
	     if ($task) { yyy(); }

See also the next item, B<-isbc>, as well as B<-sbc>, for other ways to
have some indented and some outdented block comments.

=item B<-isbc>,  B<--indent-spaced-block-comments>

If there is no leading space on the line, then the comment will not be
indented, and otherwise it may be.

If both B<-ibc> and B<-isbc> are set, then B<-isbc> takes priority.

=item B<-olc>, B<--outdent-long-comments>

When B<-olc> is set, lines which are full-line (block) comments longer
than the value B<maximum-line-length> will have their indentation
removed.  This is the default; use B<-nolc> to prevent outdenting.

=item B<-msc=n>,  B<--minimum-space-to-comment=n>

Side comments look best when lined up several spaces to the right of
code.  Perltidy will try to keep comments at least n spaces to the
right.  The default is n=4 spaces.

=item B<-fpsc=n>,  B<--fixed-position-side-comment=n>

This parameter tells perltidy to line up side comments in column number B<n>
whenever possible.  The default, n=0, will not do this.

=item B<-iscl>,  B<--ignore-side-comment-lengths>

This parameter causes perltidy to ignore the length of side comments when
setting line breaks.  The default, B<-niscl>, is to include the length of 
side comments when breaking lines to stay within the length prescribed
by the B<-l=n> maximum line length parameter.  For example, the following
long single line would remain intact with -l=80 and -iscl:

     perltidy -l=80 -iscl
        $vmsfile =~ s/;[\d\-]*$//; # Clip off version number; we can use a newer version as well

whereas without the -iscl flag the line will be broken:

     perltidy -l=80
        $vmsfile =~ s/;[\d\-]*$//
          ;    # Clip off version number; we can use a newer version as well
   

=item B<-hsc>, B<--hanging-side-comments>

By default, perltidy tries to identify and align "hanging side
comments", which are something like this:

        my $IGNORE = 0;    # This is a side comment
                           # This is a hanging side comment
                           # And so is this

A comment is considered to be a hanging side comment if (1) it immediately
follows a line with a side comment, or another hanging side comment, and
(2) there is some leading whitespace on the line.
To deactivate this feature, use B<-nhsc> or B<--nohanging-side-comments>.  
If block comments are preceded by a blank line, or have no leading
whitespace, they will not be mistaken as hanging side comments.

=item Closing Side Comments

A closing side comment is a special comment which perltidy can
automatically create and place after the closing brace of a code block.
They can be useful for code maintenance and debugging.  The command
B<-csc> (or B<--closing-side-comments>) adds or updates closing side
comments.  For example, here is a small code snippet

        sub message {
            if ( !defined( $_[0] ) ) {
                print("Hello, World\n");
            }
            else {
                print( $_[0], "\n" );
            }
        }

And here is the result of processing with C<perltidy -csc>:

        sub message {
            if ( !defined( $_[0] ) ) {
                print("Hello, World\n");
            }
            else {
                print( $_[0], "\n" );
            }
        } ## end sub message

A closing side comment was added for C<sub message> in this case, but not
for the C<if> and C<else> blocks, because they were below the 6 line
cutoff limit for adding closing side comments.  This limit may be
changed with the B<-csci> command, described below.

The command B<-dcsc> (or B<--delete-closing-side-comments>) reverses this 
process and removes these comments.

Several commands are available to modify the behavior of these two basic
commands, B<-csc> and B<-dcsc>:

=over 4

=item B<-csci=n>, or B<--closing-side-comment-interval=n> 

where C<n> is the minimum number of lines that a block must have in
order for a closing side comment to be added.  The default value is
C<n=6>.  To illustrate:

        # perltidy -csci=2 -csc
        sub message {
            if ( !defined( $_[0] ) ) {
                print("Hello, World\n");
            } ## end if ( !defined( $_[0] ))
            else {
                print( $_[0], "\n" );
            } ## end else [ if ( !defined( $_[0] ))
        } ## end sub message

Now the C<if> and C<else> blocks are commented.  However, now this has
become very cluttered.

=item B<-cscp=string>, or B<--closing-side-comment-prefix=string> 

where string is the prefix used before the name of the block type.  The
default prefix, shown above, is C<## end>.  This string will be added to
closing side comments, and it will also be used to recognize them in
order to update, delete, and format them.  Any comment identified as a
closing side comment will be placed just a single space to the right of
its closing brace.

=item B<-cscl=string>, or B<--closing-side-comment-list-string> 

where C<string> is a list of block types to be tagged with closing side
comments.  By default, all code block types preceded by a keyword or
label (such as C<if>, C<sub>, and so on) will be tagged.  The B<-cscl>
command changes the default list to be any selected block types; see
L<Specifying Block Types>.
For example, the following command
requests that only C<sub>'s, labels, C<BEGIN>, and C<END> blocks be
affected by any B<-csc> or B<-dcsc> operation:

   -cscl="sub : BEGIN END"

=item B<-csct=n>, or B<--closing-side-comment-maximum-text=n> 

The text appended to certain block types, such as an C<if> block, is
whatever lies between the keyword introducing the block, such as C<if>,
and the opening brace.  Since this might be too much text for a side
comment, there needs to be a limit, and that is the purpose of this
parameter.  The default value is C<n=20>, meaning that no additional
tokens will be appended to this text after its length reaches 20
characters.  Omitted text is indicated with C<...>.  (Tokens, including
sub names, are never truncated, however, so actual lengths may exceed
this).  To illustrate, in the above example, the appended text of the
first block is C< ( !defined( $_[0] )...>.  The existing limit of
C<n=20> caused this text to be truncated, as indicated by the C<...>.  See
the next flag for additional control of the abbreviated text.

=item B<-cscb>, or B<--closing-side-comments-balanced> 

As discussed in the previous item, when the
closing-side-comment-maximum-text limit is exceeded the comment text must
be truncated.  Older versions of perltidy terminated with three dots, and this
can still be achieved with -ncscb:

  perltidy -csc -ncscb
  } ## end foreach my $foo (sort { $b cmp $a ...

However this causes a problem with editors which cannot recognize
comments or are not configured to do so because they cannot "bounce" around in
the text correctly.  The B<-cscb> flag has been added to
help them by appending appropriate balancing structure:

  perltidy -csc -cscb
  } ## end foreach my $foo (sort { $b cmp $a ... })

The default is B<-cscb>.

=item B<-csce=n>, or B<--closing-side-comment-else-flag=n> 

The default, B<n=0>, places the text of the opening C<if> statement after any
terminal C<else>.

If B<n=2> is used, then each C<elsif> is also given the text of the opening
C<if> statement.  Also, an C<else> will include the text of a preceding
C<elsif> statement.  Note that this may result some long closing
side comments.

If B<n=1> is used, the results will be the same as B<n=2> whenever the
resulting line length is less than the maximum allowed.
=item B<-cscb>, or B<--closing-side-comments-balanced> 

When using closing-side-comments, and the closing-side-comment-maximum-text
limit is exceeded, then the comment text must be abbreviated.  
It is terminated with three dots if the B<-cscb> flag is negated:

  perltidy -csc -ncscb
  } ## end foreach my $foo (sort { $b cmp $a ...

This causes a problem with older editors which do not recognize comments
because they cannot "bounce" around in the text correctly.  The B<-cscb>
flag tries to help them by appending appropriate terminal balancing structures:

  perltidy -csc -cscb
  } ## end foreach my $foo (sort { $b cmp $a ... })

The default is B<-cscb>.  


=item B<-cscw>, or B<--closing-side-comment-warnings> 

This parameter is intended to help make the initial transition to the use of
closing side comments.  
It causes two
things to happen if a closing side comment replaces an existing, different
closing side comment:  first, an error message will be issued, and second, the
original side comment will be placed alone on a new specially marked comment
line for later attention. 

The intent is to avoid clobbering existing hand-written side comments
which happen to match the pattern of closing side comments. This flag
should only be needed on the first run with B<-csc>.

=back

B<Important Notes on Closing Side Comments:> 

=over 4

=item *

Closing side comments are only placed on lines terminated with a closing
brace.  Certain closing styles, such as the use of cuddled elses
(B<-ce>), preclude the generation of some closing side comments.

=item *

Please note that adding or deleting of closing side comments takes
place only through the commands B<-csc> or B<-dcsc>.  The other commands,
if used, merely modify the behavior of these two commands.  

=item *

It is recommended that the B<-cscw> flag be used along with B<-csc> on
the first use of perltidy on a given file.  This will prevent loss of
any existing side comment data which happens to have the csc prefix.

=item *

Once you use B<-csc>, you should continue to use it so that any
closing side comments remain correct as code changes.  Otherwise, these
comments will become incorrect as the code is updated.

=item *

If you edit the closing side comments generated by perltidy, you must also
change the prefix to be different from the closing side comment prefix.
Otherwise, your edits will be lost when you rerun perltidy with B<-csc>.   For
example, you could simply change C<## end> to be C<## End>, since the test is
case sensitive.  You may also want to use the B<-ssc> flag to keep these
modified closing side comments spaced the same as actual closing side comments.

=item *

Temporarily generating closing side comments is a useful technique for
exploring and/or debugging a perl script, especially one written by someone
else.  You can always remove them with B<-dcsc>.

=back

=item Static Block Comments

Static block comments are block comments with a special leading pattern,
C<##> by default, which will be treated slightly differently from other
block comments.  They effectively behave as if they had glue along their
left and top edges, because they stick to the left edge and previous line
when there is no blank spaces in those places.  This option is
particularly useful for controlling how commented code is displayed.

=over 4

=item B<-sbc>, B<--static-block-comments>

When B<-sbc> is used, a block comment with a special leading pattern, C<##> by
default, will be treated specially. 

Comments so identified  are treated as follows: 

=over 4

=item *

If there is no leading space on the line, then the comment will not
be indented, and otherwise it may be,

=item *

no new blank line will be
inserted before such a comment, and 

=item *

such a comment will never become
a hanging side comment.  

=back

For example, assuming C<@month_of_year> is
left-adjusted:

    @month_of_year = (    # -sbc (default)
        'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct',
    ##  'Dec', 'Nov'
        'Nov', 'Dec');

Without this convention, the above code would become

    @month_of_year = (   # -nsbc
        'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct',
  
        ##  'Dec', 'Nov'
        'Nov', 'Dec'
    );

which is not as clear.
The default is to use B<-sbc>.  This may be deactivated with B<-nsbc>.

=item B<-sbcp=string>, B<--static-block-comment-prefix=string>

This parameter defines the prefix used to identify static block comments
when the B<-sbc> parameter is set.  The default prefix is C<##>,
corresponding to C<-sbcp=##>.  The prefix is actually part of a perl 
pattern used to match lines and it must either begin with C<#> or C<^#>.  
In the first case a prefix ^\s* will be added to match any leading
whitespace, while in the second case the pattern will match only
comments with no leading whitespace.  For example, to
identify all comments as static block comments, one would use C<-sbcp=#>.
To identify all left-adjusted comments as static block comments, use C<-sbcp='^#'>.

Please note that B<-sbcp> merely defines the pattern used to identify static
block comments; it will not be used unless the switch B<-sbc> is set.  Also,
please be aware that since this string is used in a perl regular expression
which identifies these comments, it must enable a valid regular expression to
be formed.

A pattern which can be useful is:

    -sbcp=^#{2,}[^\s#] 

This pattern requires a static block comment to have at least one character
which is neither a # nor a space.  It allows a line containing only '#'
characters to be rejected as a static block comment.  Such lines are often used
at the start and end of header information in subroutines and should not be
separated from the intervening comments, which typically begin with just a
single '#'.

=item B<-osbc>, B<--outdent-static-block-comments>

The command B<-osbc> will cause static block comments to be outdented by 2
spaces (or whatever B<-ci=n> has been set to), if possible.

=back

=item Static Side Comments

Static side comments are side comments with a special leading pattern.
This option can be useful for controlling how commented code is displayed
when it is a side comment.

=over 4

=item B<-ssc>, B<--static-side-comments>

When B<-ssc> is used, a side comment with a static leading pattern, which is
C<##> by default, will be spaced only a single space from previous
character, and it will not be vertically aligned with other side comments.

The default is B<-nssc>.

=item B<-sscp=string>, B<--static-side-comment-prefix=string>

This parameter defines the prefix used to identify static side comments
when the B<-ssc> parameter is set.  The default prefix is C<##>,
corresponding to C<-sscp=##>.  

Please note that B<-sscp> merely defines the pattern used to identify
static side comments; it will not be used unless the switch B<-ssc> is
set.  Also, note that this string is used in a perl regular expression
which identifies these comments, so it must enable a valid regular
expression to be formed.

=back


=back

=head2 Skipping Selected Sections of Code

Selected lines of code may be passed verbatim to the output without any
formatting.  This feature is enabled by default but can be disabled with
the B<--noformat-skipping> or B<-nfs> flag.  It should be used sparingly to
avoid littering code with markers, but it might be helpful for working
around occasional problems.  For example it might be useful for keeping
the indentation of old commented code unchanged, keeping indentation of
long blocks of aligned comments unchanged, keeping certain list
formatting unchanged, or working around a glitch in perltidy.

=over 4

=item B<-fs>,  B<--format-skipping>

This flag, which is enabled by default, causes any code between
special beginning and ending comment markers to be passed to the
output without formatting.  The default beginning marker is #<<<
and the default ending marker is #>>> but they
may be changed (see next items below).  Additional text may appear on
these special comment lines provided that it is separated from the
marker by at least one space.  For example

 #<<<  do not let perltidy touch this
    my @list = (1,
                1, 1,
                1, 2, 1,
                1, 3, 3, 1,
                1, 4, 6, 4, 1,);
 #>>>

The comment markers may be placed at any location that a block comment may
appear.  If they do not appear to be working, use the -log flag and examine the
F<.LOG> file.  Use B<-nfs> to disable this feature.

=item B<-fsb=string>,  B<--format-skipping-begin=string>

The B<-fsb=string> parameter may be used to change the beginning marker for
format skipping.  The default is equivalent to -fsb='#<<<'.  The string that
you enter must begin with a # and should be in quotes as necessary to get past
the command shell of your system.  It is actually the leading text of a pattern
that is constructed by appending a '\s', so you must also include backslashes
for characters to be taken literally rather than as patterns.  

Some examples show how example strings become patterns:

 -fsb='#\{\{\{' becomes /^#\{\{\{\s/  which matches  #{{{ but not #{{{{
 -fsb='#\*\*'   becomes /^#\*\*\s/    which matches  #** but not #***
 -fsb='#\*{2,}' becomes /^#\*{2,}\s/  which matches  #** and #***** 

=item B<-fse=string>,  B<--format-skipping-end=string>

The B<-fsb=string> is the corresponding parameter used to change the
ending marker for format skipping.  The default is equivalent to
-fse='#<<<'.  

=back

=head2 Line Break Control

The parameters in this section control breaks after
non-blank lines of code.  Blank lines are controlled
separately by parameters in the section L<Blank Line
Control>.

=over 4

=item B<-fnl>,  B<--freeze-newlines>

If you do not want any changes to the line breaks within
lines of code in your script, set
B<-fnl>, and they will remain fixed, and the rest of the commands in
this section and sections 
L<Controlling List Formatting>,
L<Retaining or Ignoring Existing Line Breaks>. 
You may want to use B<-noll> with this.

Note: If you also want to keep your blank lines exactly
as they are, you can use the B<-fbl> flag which is described
in the section L<Blank Line Control>.

=item B<-ce>,   B<--cuddled-else>

Enable the "cuddled else" style, in which C<else> and C<elsif> are
follow immediately after the curly brace closing the previous block.
The default is not to use cuddled elses, and is indicated with the flag
B<-nce> or B<--nocuddled-else>.  Here is a comparison of the
alternatives:

  if ($task) {
      yyy();
  } else {    # -ce
      zzz();
  }

  if ($task) {
	yyy();
  }
  else {    # -nce  (default)
	zzz();
  }

=item B<-bl>,    B<--opening-brace-on-new-line>     

Use the flag B<-bl> to place the opening brace on a new line:

  if ( $input_file eq '-' )    # -bl 
  {                          
      important_function();
  }

This flag applies to all structural blocks, including named sub's (unless
the B<-sbl> flag is set -- see next item).

The default style, B<-nbl>, places an opening brace on the same line as
the keyword introducing it.  For example,

  if ( $input_file eq '-' ) {   # -nbl (default)

=item B<-sbl>,    B<--opening-sub-brace-on-new-line>     

The flag B<-sbl> can be used to override the value of B<-bl> for
the opening braces of named sub's.  For example, 

 perltidy -sbl

produces this result:

 sub message
 {
    if (!defined($_[0])) {
        print("Hello, World\n");
    }
    else {
        print($_[0], "\n");
    }
 }

This flag is negated with B<-nsbl>.  If B<-sbl> is not specified,
the value of B<-bl> is used.

=item B<-asbl>,    B<--opening-anonymous-sub-brace-on-new-line>     

The flag B<-asbl> is like the B<-sbl> flag except that it applies
to anonymous sub's instead of named subs. For example

 perltidy -asbl

produces this result:

 $a = sub
 {
     if ( !defined( $_[0] ) ) {
         print("Hello, World\n");
     }
     else {
         print( $_[0], "\n" );
     }
 };

This flag is negated with B<-nasbl>, and the default is B<-nasbl>.

=item B<-bli>,    B<--brace-left-and-indent>     

The flag B<-bli> is the same as B<-bl> but in addition it causes one 
unit of continuation indentation ( see B<-ci> ) to be placed before 
an opening and closing block braces.

For example,

        if ( $input_file eq '-' )    # -bli
          {
            important_function();
          }

By default, this extra indentation occurs for blocks of type:
B<if>, B<elsif>, B<else>, B<unless>, B<for>, B<foreach>, B<sub>, 
B<while>, B<until>, and also with a preceding label.  The next item
shows how to change this.

=item B<-blil=s>,    B<--brace-left-and-indent-list=s>     

Use this parameter to change the types of block braces for which the
B<-bli> flag applies; see L<Specifying Block Types>.  For example,
B<-blil='if elsif else'> would apply it to only C<if/elsif/else> blocks.

=item B<-bar>,    B<--opening-brace-always-on-right>     

The default style, B<-nbl> places the opening code block brace on a new
line if it does not fit on the same line as the opening keyword, like
this:

        if ( $bigwasteofspace1 && $bigwasteofspace2
          || $bigwasteofspace3 && $bigwasteofspace4 )
        {
            big_waste_of_time();
        }

To force the opening brace to always be on the right, use the B<-bar>
flag.  In this case, the above example becomes

        if ( $bigwasteofspace1 && $bigwasteofspace2
          || $bigwasteofspace3 && $bigwasteofspace4 ) {
            big_waste_of_time();
        }

A conflict occurs if both B<-bl> and B<-bar> are specified.

=item B<-otr>,  B<--opening-token-right> and related flags

The B<-otr> flag is a hint that perltidy should not place a break between a
comma and an opening token.  For example:

    # default formatting
    push @{ $self->{$module}{$key} },
      {
        accno       => $ref->{accno},
        description => $ref->{description}
      };

    # perltidy -otr
    push @{ $self->{$module}{$key} }, {
        accno       => $ref->{accno},
        description => $ref->{description}
      };

The flag B<-otr> is actually an abbreviation for three other flags
which can be used to control parens, hash braces, and square brackets
separately if desired:

  -opr  or --opening-paren-right
  -ohbr or --opening-hash-brace-right
  -osbr or --opening-square-bracket-right

=item Vertical tightness of non-block curly braces, parentheses, and square brackets.

These parameters control what shall be called vertical tightness.  Here are the
main points:

=over 4

=item *

Opening tokens (except for block braces) are controlled by B<-vt=n>, or
B<--vertical-tightness=n>, where

 -vt=0 always break a line after opening token (default). 
 -vt=1 do not break unless this would produce more than one 
         step in indentation in a line.
 -vt=2 never break a line after opening token

=item *

You must also use the B<-lp> flag when you use the B<-vt> flag; the
reason is explained below.

=item *

Closing tokens (except for block braces) are controlled by B<-vtc=n>, or
B<--vertical-tightness-closing=n>, where

 -vtc=0 always break a line before a closing token (default), 
 -vtc=1 do not break before a closing token which is followed 
        by a semicolon or another closing token, and is not in 
        a list environment.
 -vtc=2 never break before a closing token.

The rules for B<-vtc=1> are designed to maintain a reasonable balance
between tightness and readability in complex lists.

=item *

Different controls may be applied to different token types,
and it is also possible to control block braces; see below.

=item *

Finally, please note that these vertical tightness flags are merely
hints to the formatter, and it cannot always follow them.  Things which
make it difficult or impossible include comments, blank lines, blocks of
code within a list, and possibly the lack of the B<-lp> parameter.
Also, these flags may be ignored for very small lists (2 or 3 lines in
length).

=back

Here are some examples: 

    # perltidy -lp -vt=0 -vtc=0
    %romanNumerals = (
                       one   => 'I',
                       two   => 'II',
                       three => 'III',
                       four  => 'IV',
    );

    # perltidy -lp -vt=1 -vtc=0
    %romanNumerals = ( one   => 'I',
                       two   => 'II',
                       three => 'III',
                       four  => 'IV',
    );

    # perltidy -lp -vt=1 -vtc=1
    %romanNumerals = ( one   => 'I',
                       two   => 'II',
                       three => 'III',
                       four  => 'IV', );

The difference between B<-vt=1> and B<-vt=2> is shown here:

    # perltidy -lp -vt=1 
    $init->add(
                mysprintf( "(void)find_threadsv(%s);",
                           cstring( $threadsv_names[ $op->targ ] )
                )
    );

    # perltidy -lp -vt=2 
    $init->add( mysprintf( "(void)find_threadsv(%s);",
                           cstring( $threadsv_names[ $op->targ ] )
                )
    );

With B<-vt=1>, the line ending in C<add(> does not combine with the next
line because the next line is not balanced.  This can help with
readability, but B<-vt=2> can be used to ignore this rule.

The tightest, and least readable, code is produced with both C<-vt=2> and
C<-vtc=2>:

    # perltidy -lp -vt=2 -vtc=2
    $init->add( mysprintf( "(void)find_threadsv(%s);",
                           cstring( $threadsv_names[ $op->targ ] ) ) );

Notice how the code in all of these examples collapses vertically as
B<-vt> increases, but the indentation remains unchanged.  This is
because perltidy implements the B<-vt> parameter by first formatting as
if B<-vt=0>, and then simply overwriting one output line on top of the
next, if possible, to achieve the desired vertical tightness.  The
B<-lp> indentation style has been designed to allow this vertical
collapse to occur, which is why it is required for the B<-vt> parameter.

The B<-vt=n> and B<-vtc=n> parameters apply to each type of container
token.  If desired, vertical tightness controls can be applied
independently to each of the closing container token types.

The parameters for controlling parentheses are B<-pvt=n> or
B<--paren-vertical-tightness=n>, and B<-pcvt=n> or
B<--paren-vertical-tightness-closing=n>.

Likewise, the parameters for square brackets are B<-sbvt=n> or
B<--square-bracket-vertical-tightness=n>, and B<-sbcvt=n> or
B<--square-bracket-vertical-tightness-closing=n>.

Finally, the parameters for controlling non-code block braces are
B<-bvt=n> or B<--brace-vertical-tightness=n>, and B<-bcvt=n> or
B<--brace-vertical-tightness-closing=n>.

In fact, the parameter B<-vt=n> is actually just an abbreviation for
B<-pvt=n -bvt=n sbvt=n>, and likewise B<-vtc=n> is an abbreviation
for B<-pvtc=n -bvtc=n sbvtc=n>.

=item B<-bbvt=n> or B<--block-brace-vertical-tightness=n>

The B<-bbvt=n> flag is just like the B<-vt=n> flag but applies
to opening code block braces.

 -bbvt=0 break after opening block brace (default). 
 -bbvt=1 do not break unless this would produce more than one 
         step in indentation in a line.
 -bbvt=2 do not break after opening block brace.

It is necessary to also use either B<-bl> or B<-bli> for this to work,
because, as with other vertical tightness controls, it is implemented by
simply overwriting a line ending with an opening block brace with the
subsequent line.  For example:

    # perltidy -bli -bbvt=0
    if ( open( FILE, "< $File" ) )
      {
        while ( $File = <FILE> )
          {
            $In .= $File;
            $count++;
          }
        close(FILE);
      }

    # perltidy -bli -bbvt=1
    if ( open( FILE, "< $File" ) )
      { while ( $File = <FILE> )
          { $In .= $File;
            $count++;
          }
        close(FILE);
      }

By default this applies to blocks associated with keywords B<if>,
B<elsif>, B<else>, B<unless>, B<for>, B<foreach>, B<sub>, B<while>,
B<until>, and also with a preceding label.  This can be changed with
the parameter B<-bbvtl=string>, or
B<--block-brace-vertical-tightness-list=string>, where B<string> is a
space-separated list of block types.  For more information on the
possible values of this string, see L<Specifying Block Types>

For example, if we want to just apply this style to C<if>,
C<elsif>, and C<else> blocks, we could use 
C<perltidy -bli -bbvt=1 -bbvtl='if elsif else'>.

There is no vertical tightness control for closing block braces; with
one exception they will be placed on separate lines.
The exception is that a cascade of closing block braces may
be stacked on a single line.  See B<-scbb>.

=item B<-sot>,  B<--stack-opening-tokens> and related flags

The B<-sot> flag tells perltidy to "stack" opening tokens
when possible to avoid lines with isolated opening tokens.

For example:

    # default
    $opt_c = Text::CSV_XS->new(
        {
            binary       => 1,
            sep_char     => $opt_c,
            always_quote => 1,
        }
    );

    # -sot
    $opt_c = Text::CSV_XS->new( {
            binary       => 1,
            sep_char     => $opt_c,
            always_quote => 1,
        }
    );

For detailed control of individual closing tokens the following
controls can be used:

  -sop  or --stack-opening-paren
  -sohb or --stack-opening-hash-brace
  -sosb or --stack-opening-square-bracket
  -sobb or --stack-opening-block-brace

The flag B<-sot> is an abbreviation for B<-sop -sohb -sosb>.

The flag B<-sobb> is a abbreviation for B<-bbvt=2 -bbvtl='*'>.  This
will case a cascade of opening block braces to appear on a single line,
although this an uncommon occurrence except in test scripts. 

=item B<-sct>,  B<--stack-closing-tokens> and related flags

The B<-sct> flag tells perltidy to "stack" closing tokens
when possible to avoid lines with isolated closing tokens.

For example:

    # default
    $opt_c = Text::CSV_XS->new(
        {
            binary       => 1,
            sep_char     => $opt_c,
            always_quote => 1,
        }
    );

    # -sct
    $opt_c = Text::CSV_XS->new(
        {
            binary       => 1,
            sep_char     => $opt_c,
            always_quote => 1,
        } );

The B<-sct> flag is somewhat similar to the B<-vtc> flags, and in some
cases it can give a similar result.  The difference is that the B<-vtc>
flags try to avoid lines with leading opening tokens by "hiding" them at
the end of a previous line, whereas the B<-sct> flag merely tries to
reduce the number of lines with isolated closing tokens by stacking them
but does not try to hide them.  For example:

    # -vtc=2
    $opt_c = Text::CSV_XS->new(
        {
            binary       => 1,
            sep_char     => $opt_c,
            always_quote => 1, } );

For detailed control of the stacking of individual closing tokens the
following controls can be used:

  -scp  or --stack-closing-paren
  -schb or --stack-closing-hash-brace
  -scsb or --stack-closing-square-bracket
  -scbb or --stack-closing-block-brace

The flag B<-sct> is an abbreviation for stacking the non-block closing
tokens, B<-scp -schb -scsb>. 

Stacking of closing block braces, B<-scbb>, causes a cascade of isolated
closing block braces to be combined into a single line as in the following
example:

    # -scbb:
    for $w1 (@w1) {
        for $w2 (@w2) {
            for $w3 (@w3) {
                for $w4 (@w4) {
                    push( @lines, "$w1 $w2 $w3 $w4\n" );
                } } } }

To simplify input even further for the case in which both opening and closing
non-block containers are stacked, the flag B<-sac> or B<--stack-all-containers>
is an abbreviation for B<-sot -sot>.

=item B<-dnl>,  B<--delete-old-newlines>

By default, perltidy first deletes all old line break locations, and then it
looks for good break points to match the desired line length.  Use B<-ndnl>
or  B<--nodelete-old-newlines> to force perltidy to retain all old line break
points.  

=item B<-anl>,  B<--add-newlines>

By default, perltidy will add line breaks when necessary to create
continuations of long lines and to improve the script appearance.  Use
B<-nanl> or B<--noadd-newlines> to prevent any new line breaks.  

This flag does not prevent perltidy from eliminating existing line
breaks; see B<--freeze-newlines> to completely prevent changes to line
break points.

=item Controlling whether perltidy breaks before or after operators

Four command line parameters provide some control over whether
a line break should be before or after specific token types.
Two parameters give detailed control:

B<-wba=s> or B<--want-break-after=s>, and

B<-wbb=s> or B<--want-break-before=s>.

These parameters are each followed by a quoted string, B<s>, containing
a list of token types (separated only by spaces).  No more than one of each
of these parameters should be specified, because repeating a
command-line parameter always overwrites the previous one before
perltidy ever sees it.

By default, perltidy breaks B<after> these token types:
  % + - * / x != == >= <= =~ !~ < >  | & 
  = **= += *= &= <<= &&= -= /= |= >>= ||= //= .= %= ^= x=

And perltidy breaks B<before> these token types by default:
  . << >> -> && || //

To illustrate, to cause a break after a concatenation operator, C<'.'>,
rather than before it, the command line would be

  -wba="."

As another example, the following command would cause a break before 
math operators C<'+'>, C<'-'>, C<'/'>, and C<'*'>:

  -wbb="+ - / *"

These commands should work well for most of the token types that perltidy uses
(use B<--dump-token-types> for a list).  Also try the B<-D> flag on a short
snippet of code and look at the .DEBUG file to see the tokenization.  However,
for a few token types there may be conflicts with hardwired logic which cause
unexpected results.  One example is curly braces, which should be controlled
with the parameter B<bl> provided for that purpose.

B<WARNING> Be sure to put these tokens in quotes to avoid having them
misinterpreted by your command shell.

Two additional parameters are available which, though they provide no further
capability, can simplify input are:

B<-baao> or B<--break-after-all-operators>,

B<-bbao> or B<--break-before-all-operators>.

The -baao sets the default to be to break after all of the following operators:

    % + - * / x != == >= <= =~ !~ < > | & 
    = **= += *= &= <<= &&= -= /= |= >>= ||= //= .= %= ^= x=
    . : ? && || and or err xor

and the B<-bbao> flag sets the default to break before all of these operators.
These can be used to define an initial break preference which can be fine-tuned
with the B<-wba> and B<-wbb> flags.  For example, to break before all operators
except an B<=> one could use --bbao -wba='=' rather than listing every
single perl operator except B<=> on a -wbb flag.

=back

=head2 Controlling List Formatting

Perltidy attempts to place comma-separated arrays of values in tables
which look good.  Its default algorithms usually work well, and they
have been improving with each release, but several parameters are
available to control list formatting.

=over 4

=item B<-boc>,  B<--break-at-old-comma-breakpoints>

This flag tells perltidy to try to break at all old commas.  This is not
the default.  Normally, perltidy makes a best guess at list formatting,
and seldom uses old comma breakpoints.  Usually this works well,
but consider:

    my @list = (1,
                1, 1,
                1, 2, 1,
                1, 3, 3, 1,
                1, 4, 6, 4, 1,);

The default formatting will flatten this down to one line:

    # perltidy (default)
    my @list = ( 1, 1, 1, 1, 2, 1, 1, 3, 3, 1, 1, 4, 6, 4, 1, );

which hides the structure. Using B<-boc>, plus additional flags
to retain the original style, yields

    # perltidy -boc -lp -pt=2 -vt=1 -vtc=1
    my @list = (1,
                1, 1,
                1, 2, 1,
                1, 3, 3, 1,
                1, 4, 6, 4, 1,);

A disadvantage of this flag is that all tables in the file
must already be nicely formatted.  For another possibility see
the -fs flag in L<Skipping Selected Sections of Code>.

=item B<-mft=n>,  B<--maximum-fields-per-table=n>

If the computed number of fields for any table exceeds B<n>, then it
will be reduced to B<n>.  The default value for B<n> is a large number,
40.  While this value should probably be left unchanged as a general
rule, it might be used on a small section of code to force a list to
have a particular number of fields per line, and then either the B<-boc>
flag could be used to retain this formatting, or a single comment could
be introduced somewhere to freeze the formatting in future applications
of perltidy.

    # perltidy -mft=2
    @month_of_year = (    
        'Jan', 'Feb',
        'Mar', 'Apr',
        'May', 'Jun',
        'Jul', 'Aug',
        'Sep', 'Oct',
        'Nov', 'Dec'
    );

=item B<-cab=n>,  B<--comma-arrow-breakpoints=n>

A comma which follows a comma arrow, '=>', is given special
consideration.  In a long list, it is common to break at all such
commas.  This parameter can be used to control how perltidy breaks at
these commas.  (However, it will have no effect if old comma breaks are
being forced because B<-boc> is used).  The possible values of B<n> are:

 n=0 break at all commas after =>  
 n=1 stable: break at all commas after => if container is open,
     EXCEPT FOR one-line containers
 n=2 break at all commas after =>, BUT try to form the maximum
     maximum one-line container lengths
 n=3 do not treat commas after => specially at all 
 n=4 break everything: like n=0 but ALSO break a short container with
     a => not followed by a comma when -vt=0 is used
 n=5 stable: like n=1 but ALSO break at open one-line containers when
     -vt=0 is used (default)

For example, given the following single line, perltidy by default will
not add any line breaks because it would break the existing one-line
container:

    bless { B => $B, Root => $Root } => $package;

Using B<-cab=0> will force a break after each comma-arrow item:

    # perltidy -cab=0:
    bless {
        B    => $B,
        Root => $Root
    } => $package;

If perltidy is subsequently run with this container broken, then by
default it will break after each '=>' because the container is now
broken.  To reform a one-line container, the parameter B<-cab=2> could
be used.

The flag B<-cab=3> can be used to prevent these commas from being
treated specially.  In this case, an item such as "01" => 31 is
treated as a single item in a table.  The number of fields in this table
will be determined by the same rules that are used for any other table.
Here is an example.
    
    # perltidy -cab=3
    my %last_day = (
        "01" => 31, "02" => 29, "03" => 31, "04" => 30,
        "05" => 31, "06" => 30, "07" => 31, "08" => 31,
        "09" => 30, "10" => 31, "11" => 30, "12" => 31
    );

=back

=head2 Retaining or Ignoring Existing Line Breaks

Several additional parameters are available for controlling the extent
to which line breaks in the input script influence the output script.
In most cases, the default parameter values are set so that, if a choice
is possible, the output style follows the input style.  For example, if
a short logical container is broken in the input script, then the
default behavior is for it to remain broken in the output script.

Most of the parameters in this section would only be required for a
one-time conversion of a script from short container lengths to longer
container lengths.  The opposite effect, of converting long container
lengths to shorter lengths, can be obtained by temporarily using a short
maximum line length.

=over 4

=item B<-bol>,  B<--break-at-old-logical-breakpoints>

By default, if a logical expression is broken at a C<&&>, C<||>, C<and>,
or C<or>, then the container will remain broken.  Also, breaks
at internal keywords C<if> and C<unless> will normally be retained.
To prevent this, and thus form longer lines, use B<-nbol>.

=item B<-bok>,  B<--break-at-old-keyword-breakpoints>

By default, perltidy will retain a breakpoint before keywords which may
return lists, such as C<sort> and <map>.  This allows chains of these
operators to be displayed one per line.  Use B<-nbok> to prevent
retaining these breakpoints.

=item B<-bot>,  B<--break-at-old-ternary-breakpoints>

By default, if a conditional (ternary) operator is broken at a C<:>,
then it will remain broken.  To prevent this, and thereby
form longer lines, use B<-nbot>.

=item B<-boa>,  B<--break-at-old-attribute-breakpoints>

By default, if an attribute list is broken at a C<:> in the source file, then
it will remain broken.  For example, given the following code, the line breaks
at the ':'s will be retained:
       
                    my @field
                      : field
                      : Default(1)
                      : Get('Name' => 'foo') : Set('Name');

If the attributes are on a single line in the source code then they will remain
on a single line if possible.

To prevent this, and thereby always form longer lines, use B<-nboa>.  

=item B<-iob>,  B<--ignore-old-breakpoints>

Use this flag to tell perltidy to ignore existing line breaks to the
maximum extent possible.  This will tend to produce the longest possible
containers, regardless of type, which do not exceed the line length
limit.

=item B<-kis>,  B<--keep-interior-semicolons>

Use the B<-kis> flag to prevent breaking at a semicolon if
there was no break there in the input file.  Normally
perltidy places a newline after each semicolon which
terminates a statement unless several statements are
contained within a one-line brace block.  To illustrate,
consider the following input lines:

    dbmclose(%verb_delim); undef %verb_delim;
    dbmclose(%expanded); undef %expanded;

The default is to break after each statement, giving

    dbmclose(%verb_delim);
    undef %verb_delim;
    dbmclose(%expanded);
    undef %expanded;

With B<perltidy -kis> the multiple statements are retained:

    dbmclose(%verb_delim); undef %verb_delim;
    dbmclose(%expanded);   undef %expanded;

The statements are still subject to the specified value
of B<maximum-line-length> and will be broken if this 
maximum is exceeded.

=back

=head2 Blank Line Control

Blank lines can improve the readability of a script if they are carefully
placed.  Perltidy has several commands for controlling the insertion,
retention, and removal of blank lines.  

=over 4

=item B<-fbl>,  B<--freeze-blank-lines>

Set B<-fbl> if you want to the blank lines in your script to
remain exactly as they are.  The rest of the parameters in
this section may then be ignored.  (Note: setting the B<-fbl> flag
is equivalent to setting B<-mbl=0> and B<-kbl=2>).

=item B<-bbc>,  B<--blanks-before-comments>

A blank line will be introduced before a full-line comment.  This is the
default.  Use B<-nbbc> or  B<--noblanks-before-comments> to prevent
such blank lines from being introduced.

=item B<-blbs=n>,  B<--blank-lines-before-subs=n>

The parameter B<-blbs=n> requests that least B<n> blank lines precede a sub
definition which does not follow a comment and which is more than one-line
long.  The default is <-blbs=1>.  B<BEGIN> and B<END> blocks are included.

The requested number of blanks statement will be inserted regardless of the
value of B<--maximum-consecutive-blank-lines=n> (B<-mbl=n>) with the exception
that if B<-mbl=0> then no blanks will be output.

This parameter interacts with the value B<k> of the parameter B<--maximum-consecutive-blank-lines=k> (B<-mbl=k>) as follows:

1. If B<-mbl=0> then no blanks will be output.  This allows all blanks to be suppressed with a single parameter.  Otherwise,

2. If the number of old blank lines in the script is less than B<n> then
additional blanks will be inserted to make the total B<n> regardless of the
value of B<-mbl=k>.  

3. If the number of old blank lines in the script equals or exceeds B<n> then
this parameter has no effect, however the total will not exceed
value specified on the B<-mbl=k> flag.


=item B<-blbp=n>,  B<--blank-lines-before-packages=n>

The parameter B<-blbp=n> requests that least B<n> blank lines precede a package
which does not follow a comment.  The default is <-blbp=1>.  

This parameter interacts with the value B<k> of the parameter
B<--maximum-consecutive-blank-lines=k> (B<-mbl=k>) in the same way as described
for the previous item B<-blbs=n>.


=item B<-bbs>,  B<--blanks-before-subs>

For compatibility with previous versions, B<-bbs> or B<--blanks-before-subs>
is equivalent to F<-blbp=1> and F<-blbs=1>.  

Likewise, B<-nbbs> or B<--noblanks-before-subs> 
is equivalent to F<-blbp=0> and F<-blbs=0>.  

=item B<-bbb>,  B<--blanks-before-blocks>

A blank line will be introduced before blocks of coding delimited by
B<for>, B<foreach>, B<while>, B<until>, and B<if>, B<unless>, in the following
circumstances:

=over 4

=item *

The block is not preceded by a comment.

=item *

The block is not a one-line block.

=item *

The number of consecutive non-blank lines at the current indentation depth is at least B<-lbl>
(see next section).

=back

This is the default.  The intention of this option is to introduce
some space within dense coding.
This is negated with B<-nbbb> or  B<--noblanks-before-blocks>.

=item B<-lbl=n> B<--long-block-line-count=n>

This controls how often perltidy is allowed to add blank lines before 
certain block types (see previous section).  The default is 8.  Entering
a value of B<0> is equivalent to entering a very large number.

=item B<-mbl=n> B<--maximum-consecutive-blank-lines=n>   

This parameter specifies the maximum number of consecutive blank lines which
will be output within code sections of a script.  The default is n=1.  If the
input file has more than n consecutive blank lines, the number will be reduced
to n except as noted above for the B<-blbp> and B<-blbs> parameters.  If B<n=0>
then no blank lines will be output (unless all old blank lines are retained
with the B<-kbl=2> flag of the next section).

This flag obviously does not apply to pod sections,
here-documents, and quotes.  

=item B<-kbl=n>,  B<--keep-old-blank-lines=n>

The B<-kbl=n> flag gives you control over how your existing blank lines are
treated.  

The possible values of B<n> are:

 n=0 ignore all old blank lines
 n=1 stable: keep old blanks, but limited by the value of the B<-mbl=n> flag
 n=2 keep all old blank lines, regardless of the value of the B<-mbl=n> flag

The default is B<n=1>.  

=item B<-sob>,  B<--swallow-optional-blank-lines>

This is equivalent to B<kbl=0> and is included for compatibility with
previous versions.

=item B<-nsob>,  B<--noswallow-optional-blank-lines>

This is equivalent to B<kbl=1> and is included for compatibility with
previous versions.

=back

=head2 Styles

A style refers to a convenient collection of existing parameters.

=over 4

=item B<-gnu>, B<--gnu-style>

B<-gnu> gives an approximation to the GNU Coding Standards (which do
not apply to perl) as they are sometimes implemented.  At present, this
style overrides the default style with the following parameters:

    -lp -bl -noll -pt=2 -bt=2 -sbt=2 -icp

=item B<-pbp>, B<--perl-best-practices>

B<-pbp> is an abbreviation for the parameters in the book B<Perl Best Practices>
by Damian Conway:

    -l=78 -i=4 -ci=4 -st -se -vt=2 -cti=0 -pt=1 -bt=1 -sbt=1 -bbt=1 -nsfs -nolq
    -wbb="% + - * / x != == >= <= =~ !~ < > | & = 
          **= += *= &= <<= &&= -= /= |= >>= ||= //= .= %= ^= x="

Please note that this parameter set includes -st and -se flags, which make
perltidy act as a filter on one file only.  These can be overridden by placing
B<-nst> and/or B<-nse> after the -pbp parameter. 

Also note that the value of continuation indentation, -ci=4, is equal to the
value of the full indentation, -i=4.  In some complex statements perltidy will
produce nicer results with -ci=2. This can be implemented by including -ci=2
after the -pbp parameter.  For example, 

    # perltidy -pbp
    $self->{_text} = (
         !$section        ? ''
        : $type eq 'item' ? "the $section entry"
        :                   "the section on $section"
        )
        . (
        $page
        ? ( $section ? ' in ' : '' ) . "the $page$page_ext manpage"
        : ' elsewhere in this document'
        );

    # perltidy -pbp -ci=2
    $self->{_text} = (
         !$section        ? ''
        : $type eq 'item' ? "the $section entry"
        :                   "the section on $section"
      )
      . (
        $page
        ? ( $section ? ' in ' : '' ) . "the $page$page_ext manpage"
        : ' elsewhere in this document'
      );

=back

=head2 Other Controls

=over 4

=item Deleting selected text 

Perltidy can selectively delete comments and/or pod documentation.  The
command B<-dac> or  B<--delete-all-comments> will delete all comments
B<and> all pod documentation, leaving just code and any leading system
control lines.

The command B<-dp> or B<--delete-pod> will remove all pod documentation
(but not comments).

Two commands which remove comments (but not pod) are: B<-dbc> or
B<--delete-block-comments> and B<-dsc> or  B<--delete-side-comments>.
(Hanging side comments will be deleted with block comments here.)

The negatives of these commands also work, and are the defaults.  When
block comments are deleted, any leading 'hash-bang' will be retained.
Also, if the B<-x> flag is used, any system commands before a leading
hash-bang will be retained (even if they are in the form of comments).

=item Writing selected text to a file

When perltidy writes a formatted text file, it has the ability to also
send selected text to a file with a F<.TEE> extension.  This text can
include comments and pod documentation.  

The command B<-tac> or  B<--tee-all-comments> will write all comments
B<and> all pod documentation.

The command B<-tp> or B<--tee-pod> will write all pod documentation (but
not comments).

The commands which write comments (but not pod) are: B<-tbc> or
B<--tee-block-comments> and B<-tsc> or  B<--tee-side-comments>.
(Hanging side comments will be written with block comments here.)

The negatives of these commands also work, and are the defaults.  

=item Using a F<.perltidyrc> command file

If you use perltidy frequently, you probably won't be happy until you
create a F<.perltidyrc> file to avoid typing commonly-used parameters.
Perltidy will first look in your current directory for a command file
named F<.perltidyrc>.  If it does not find one, it will continue looking
for one in other standard locations.  

These other locations are system-dependent, and may be displayed with
the command C<perltidy -dpro>.  Under Unix systems, it will first look
for an environment variable B<PERLTIDY>.  Then it will look for a
F<.perltidyrc> file in the home directory, and then for a system-wide
file F</usr/local/etc/perltidyrc>, and then it will look for
F</etc/perltidyrc>.  Note that these last two system-wide files do not
have a leading dot.  Further system-dependent information will be found
in the INSTALL file distributed with perltidy.

Under Windows, perltidy will also search for a configuration file named perltidy.ini since Windows does not allow files with a leading period (.).
Use C<perltidy -dpro> to see the possible locations for your system.
An example might be F<C:\Documents and Settings\All Users\perltidy.ini>.

Another option is the use of the PERLTIDY environment variable.
The method for setting environment variables depends upon the version of
Windows that you are using.  Instructions for Windows 95 and later versions can
be found here:

http://www.netmanage.com/000/20021101_005_tcm21-6336.pdf

Under Windows NT / 2000 / XP the PERLTIDY environment variable can be placed in
either the user section or the system section.  The later makes the
configuration file common to all users on the machine.  Be sure to enter the
full path of the configuration file in the value of the environment variable.
Ex.  PERLTIDY=C:\Documents and Settings\perltidy.ini

The configuration file is free format, and simply a list of parameters, just as
they would be entered on a command line.  Any number of lines may be used, with
any number of parameters per line, although it may be easiest to read with one
parameter per line.  Comment text begins with a #, and there must
also be a space before the # for side comments.  It is a good idea to
put complex parameters in either single or double quotes.

Here is an example of a F<.perltidyrc> file:

  # This is a simple of a .perltidyrc configuration file
  # This implements a highly spaced style
  -se    # errors to standard error output
  -w     # show all warnings
  -bl	 # braces on new lines
  -pt=0  # parens not tight at all
  -bt=0  # braces not tight
  -sbt=0 # square brackets not tight

The parameters in the F<.perltidyrc> file are installed first, so any
parameters given on the command line will have priority over them.  

To avoid confusion, perltidy ignores any command in the .perltidyrc
file which would cause some kind of dump and an exit.  These are:

 -h -v -ddf -dln -dop -dsn -dtt -dwls -dwrs -ss

There are several options may be helpful in debugging a F<.perltidyrc>
file:  

=over 4

=item *

A very helpful command is B<--dump-profile> or B<-dpro>.  It writes a
list of all configuration filenames tested to standard output, and 
if a file is found, it dumps the content to standard output before
exiting.  So, to find out where perltidy looks for its configuration
files, and which one if any it selects, just enter 

  perltidy -dpro

=item *

It may be simplest to develop and test configuration files with
alternative names, and invoke them with B<-pro=filename> on the command
line.  Then rename the desired file to F<.perltidyrc> when finished.

=item *

The parameters in the F<.perltidyrc> file can be switched off with 
the B<-npro> option.

=item *

The commands B<--dump-options>, B<--dump-defaults>, B<--dump-long-names>,
and B<--dump-short-names>, all described below, may all be helpful.

=back

=item Creating a new abbreviation

A special notation is available for use in a F<.perltidyrc> file
for creating an abbreviation for a group
of options.  This can be used to create a
shorthand for one or more styles which are frequently, but not always,
used.  The notation is to group the options within curly braces which
are preceded by the name of the alias (without leading dashes), like this:

	newword {
	-opt1
	-opt2
	}

where B<newword> is the abbreviation, and B<opt1>, etc, are existing parameters
I<or other abbreviations>.  The main syntax requirement is that the new
abbreviation along with its opening curly brace must begin on a new line.
Space before and after the curly braces is optional.
For a
specific example, the following line

	airy {-bl -pt=0 -bt=0 -sbt=0}

could be placed in a F<.perltidyrc> file, and then invoked at will with

	perltidy -airy somefile.pl

(Either C<-airy> or C<--airy> may be used).

=item Skipping leading non-perl commands with B<-x> or B<--look-for-hash-bang>

If your script has leading lines of system commands or other text which
are not valid perl code, and which are separated from the start of the
perl code by a "hash-bang" line, ( a line of the form C<#!...perl> ),
you must use the B<-x> flag to tell perltidy not to parse and format any
lines before the "hash-bang" line.  This option also invokes perl with a
-x flag when checking the syntax.  This option was originally added to
allow perltidy to parse interactive VMS scripts, but it should be used
for any script which is normally invoked with C<perl -x>.

=item  Making a file unreadable

The goal of perltidy is to improve the readability of files, but there
are two commands which have the opposite effect, B<--mangle> and
B<--extrude>.  They are actually
merely aliases for combinations of other parameters.  Both of these
strip all possible whitespace, but leave comments and pod documents,
so that they are essentially reversible.  The
difference between these is that B<--mangle> puts the fewest possible
line breaks in a script while B<--extrude> puts the maximum possible.
Note that these options do not provided any meaningful obfuscation, because
perltidy can be used to reformat the files.  They were originally
developed to help test the tokenization logic of perltidy, but they
have other uses.
One use for B<--mangle> is the following:

  perltidy --mangle myfile.pl -st | perltidy -o myfile.pl.new

This will form the maximum possible number of one-line blocks (see next
section), and can sometimes help clean up a badly formatted script.

A similar technique can be used with B<--extrude> instead of B<--mangle>
to make the minimum number of one-line blocks.

Another use for B<--mangle> is to combine it with B<-dac> to reduce
the file size of a perl script.

=item  One-line blocks 

There are a few points to note regarding one-line blocks.  A one-line
block is something like this,

	if ($x > 0) { $y = 1 / $x }  

where the contents within the curly braces is short enough to fit
on a single line.

With few exceptions, perltidy retains existing one-line blocks, if it
is possible within the line-length constraint, but it does not attempt
to form new ones.  In other words, perltidy will try to follow the
one-line block style of the input file.

If an existing one-line block is longer than the maximum line length,
however, it will be broken into multiple lines.  When this happens, perltidy
checks for and adds any optional terminating semicolon (unless the B<-nasc>
option is used) if the block is a code block.  

The main exception is that perltidy will attempt to form new one-line
blocks following the keywords C<map>, C<eval>, and C<sort>, because
these code blocks are often small and most clearly displayed in a single
line.

One-line block rules can conflict with the cuddled-else option.  When
the cuddled-else option is used, perltidy retains existing one-line
blocks, even if they do not obey cuddled-else formatting.

Occasionally, when one-line blocks get broken because they exceed the
available line length, the formatting will violate the requested brace style.
If this happens, reformatting the script a second time should correct
the problem.

=item  Debugging 

The following flags are available for debugging:

B<--dump-defaults> or B<-ddf> will write the default option set to standard output and quit

B<--dump-profile> or B<-dpro>  will write the name of the current 
configuration file and its contents to standard output and quit.

B<--dump-options> or B<-dop>  will write current option set to standard
output and quit.  

B<--dump-long-names> or B<-dln>  will write all command line long names (passed 
to Get_options) to standard output and quit.

B<--dump-short-names>  or B<-dsn> will write all command line short names 
to standard output and quit.

B<--dump-token-types> or B<-dtt>  will write a list of all token types 
to standard output and quit.

B<--dump-want-left-space> or B<-dwls>  will write the hash %want_left_space
to standard output and quit.  See the section on controlling whitespace
around tokens.

B<--dump-want-right-space> or B<-dwrs>  will write the hash %want_right_space
to standard output and quit.  See the section on controlling whitespace
around tokens.

B<--no-memoize> or B<-nmem>  will turn of memoizing.
Memoization can reduce run time when running perltidy repeatedly in a 
single process.  It is on by default but can be deactivated for
testing with B<-nmem>.

B<-DEBUG>  will write a file with extension F<.DEBUG> for each input file 
showing the tokenization of all lines of code.

=item Working with MakeMaker, AutoLoader and SelfLoader

The first $VERSION line of a file which might be eval'd by MakeMaker
is passed through unchanged except for indentation.  
Use B<--nopass-version-line>, or B<-npvl>, to deactivate this feature.

If the AutoLoader module is used, perltidy will continue formatting
code after seeing an __END__ line.
Use B<--nolook-for-autoloader>, or B<-nlal>, to deactivate this feature.

Likewise, if the SelfLoader module is used, perltidy will continue formatting
code after seeing a __DATA__ line.
Use B<--nolook-for-selfloader>, or B<-nlsl>, to deactivate this feature.

=item Working around problems with older version of Perl 

Perltidy contains a number of rules which help avoid known subtleties
and problems with older versions of perl, and these rules always
take priority over whatever formatting flags have been set.  For example,
perltidy will usually avoid starting a new line with a bareword, because
this might cause problems if C<use strict> is active.

There is no way to override these rules.

=back

=head1 HTML OPTIONS

=over 4

=item  The B<-html> master switch

The flag B<-html> causes perltidy to write an html file with extension
F<.html>.  So, for example, the following command

	perltidy -html somefile.pl

will produce a syntax-colored html file named F<somefile.pl.html>
which may be viewed with a browser.

B<Please Note>: In this case, perltidy does not do any formatting to the
input file, and it does not write a formatted file with extension
F<.tdy>.  This means that two perltidy runs are required to create a
fully reformatted, html copy of a script.  

=item  The B<-pre> flag for code snippets

When the B<-pre> flag is given, only the pre-formatted section, within
the <PRE> and </PRE> tags, will be output.  This simplifies inclusion
of the output in other files.  The default is to output a complete
web page.

=item  The B<-nnn> flag for line numbering

When the B<-nnn> flag is given, the output lines will be numbered.

=item  The B<-toc>, or B<--html-table-of-contents> flag

By default, a table of contents to packages and subroutines will be
written at the start of html output.  Use B<-ntoc> to prevent this.
This might be useful, for example, for a pod document which contains a
number of unrelated code snippets.  This flag only influences the code
table of contents; it has no effect on any table of contents produced by
pod2html (see next item).

=item  The B<-pod>, or B<--pod2html> flag

There are two options for formatting pod documentation.  The default is
to pass the pod through the Pod::Html module (which forms the basis of
the pod2html utility).  Any code sections are formatted by perltidy, and
the results then merged.  Note: perltidy creates a temporary file when
Pod::Html is used; see L<"FILES">.  Also, Pod::Html creates temporary
files for its cache.

NOTE: Perltidy counts the number of C<=cut> lines, and either moves the
pod text to the top of the html file if there is one C<=cut>, or leaves
the pod text in its original order (interleaved with code) otherwise.

Most of the flags accepted by pod2html may be included in the perltidy
command line, and they will be passed to pod2html.  In some cases,
the flags have a prefix C<pod> to emphasize that they are for the
pod2html, and this prefix will be removed before they are passed to
pod2html.  The flags which have the additional C<pod> prefix are:

   --[no]podheader --[no]podindex --[no]podrecurse --[no]podquiet 
   --[no]podverbose --podflush

The flags which are unchanged from their use in pod2html are:

   --backlink=s --cachedir=s --htmlroot=s --libpods=s --title=s
   --podpath=s --podroot=s 

where 's' is an appropriate character string.  Not all of these flags are
available in older versions of Pod::Html.  See your Pod::Html documentation for
more information.

The alternative, indicated with B<-npod>, is not to use Pod::Html, but
rather to format pod text in italics (or whatever the stylesheet
indicates), without special html markup.  This is useful, for example,
if pod is being used as an alternative way to write comments.

=item  The B<-frm>, or B<--frames> flag

By default, a single html output file is produced.  This can be changed
with the B<-frm> option, which creates a frame holding a table of
contents in the left panel and the source code in the right side. This
simplifies code browsing.  Assume, for example, that the input file is
F<MyModule.pm>.  Then, for default file extension choices, these three
files will be created:

 MyModule.pm.html      - the frame
 MyModule.pm.toc.html  - the table of contents
 MyModule.pm.src.html  - the formatted source code

Obviously this file naming scheme requires that output be directed to a real
file (as opposed to, say, standard output).  If this is not the
case, or if the file extension is unknown, the B<-frm> option will be
ignored.

=item  The B<-text=s>, or B<--html-toc-extension> flag

Use this flag to specify the extra file extension of the table of contents file
when html frames are used.  The default is "toc".
See L<Specifying File Extensions>.

=item  The B<-sext=s>, or B<--html-src-extension> flag

Use this flag to specify the extra file extension of the content file when html
frames are used.  The default is "src".
See L<Specifying File Extensions>.

=item  The B<-hent>, or B<--html-entities> flag

This flag controls the use of Html::Entities for html formatting.  By
default, the module Html::Entities is used to encode special symbols.
This may not be the right thing for some browser/language
combinations.  Use --nohtml-entities or -nhent to prevent this.

=item  Style Sheets

Style sheets make it very convenient to control and adjust the
appearance of html pages.  The default behavior is to write a page of
html with an embedded style sheet.

An alternative to an embedded style sheet is to create a page with a
link to an external style sheet.  This is indicated with the
B<-css=filename>,  where the external style sheet is F<filename>.  The
external style sheet F<filename> will be created if and only if it does
not exist.  This option is useful for controlling multiple pages from a
single style sheet.

To cause perltidy to write a style sheet to standard output and exit,
use the B<-ss>, or B<--stylesheet>, flag.  This is useful if the style
sheet could not be written for some reason, such as if the B<-pre> flag
was used.  Thus, for example,
  
  perltidy -html -ss >mystyle.css

will write a style sheet with the default properties to file
F<mystyle.css>.

The use of style sheets is encouraged, but a web page without a style
sheets can be created with the flag B<-nss>.  Use this option if you
must to be sure that older browsers (roughly speaking, versions prior to
4.0 of Netscape Navigator and Internet Explorer) can display the
syntax-coloring of the html files.

=item  Controlling HTML properties

Note: It is usually more convenient to accept the default properties
and then edit the stylesheet which is produced.  However, this section
shows how to control the properties with flags to perltidy.

Syntax colors may be changed from their default values by flags of the either
the long form, B<-html-color-xxxxxx=n>, or more conveniently the short form,
B<-hcx=n>, where B<xxxxxx> is one of the following words, and B<x> is the
corresponding abbreviation:

      Token Type             xxxxxx           x 
      ----------             --------         --
      comment                comment          c
      number                 numeric          n
      identifier             identifier       i
      bareword, function     bareword         w
      keyword                keyword          k
      quite, pattern         quote            q
      here doc text          here-doc-text    h
      here doc target        here-doc-target  hh
      punctuation            punctuation      pu
      parentheses            paren            p
      structural braces      structure        s
      semicolon              semicolon        sc
      colon                  colon            co
      comma                  comma            cm
      label                  label            j
      sub definition name    subroutine       m
      pod text               pod-text         pd

A default set of colors has been defined, but they may be changed by providing
values to any of the following parameters, where B<n> is either a 6 digit 
hex RGB color value or an ascii name for a color, such as 'red'.

To illustrate, the following command will produce an html 
file F<somefile.pl.html> with "aqua" keywords:

	perltidy -html -hck=00ffff somefile.pl

and this should be equivalent for most browsers:

	perltidy -html -hck=aqua somefile.pl

Perltidy merely writes any non-hex names that it sees in the html file.
The following 16 color names are defined in the HTML 3.2 standard:

	black   => 000000,
	silver  => c0c0c0,
	gray    => 808080,
	white   => ffffff,
	maroon  => 800000,
	red     => ff0000,
	purple  => 800080,
	fuchsia => ff00ff,
	green   => 008000,
	lime    => 00ff00,
	olive   => 808000,
	yellow  => ffff00
	navy    => 000080,
	blue    => 0000ff,
	teal    => 008080,
	aqua    => 00ffff,

Many more names are supported in specific browsers, but it is safest
to use the hex codes for other colors.  Helpful color tables can be
located with an internet search for "HTML color tables". 

Besides color, two other character attributes may be set: bold, and italics.
To set a token type to use bold, use the flag
B<--html-bold-xxxxxx> or B<-hbx>, where B<xxxxxx> or B<x> are the long
or short names from the above table.  Conversely, to set a token type to 
NOT use bold, use B<--nohtml-bold-xxxxxx> or B<-nhbx>.

Likewise, to set a token type to use an italic font, use the flag
B<--html-italic-xxxxxx> or B<-hix>, where again B<xxxxxx> or B<x> are the
long or short names from the above table.  And to set a token type to
NOT use italics, use B<--nohtml-italic-xxxxxx> or B<-nhix>.

For example, to use bold braces and lime color, non-bold, italics keywords the
following command would be used:

	perltidy -html -hbs -hck=00FF00 -nhbk -hik somefile.pl

The background color can be specified with B<--html-color-background=n>,
or B<-hcbg=n> for short, where n is a 6 character hex RGB value.  The
default color of text is the value given to B<punctuation>, which is
black as a default.

Here are some notes and hints:

1. If you find a preferred set of these parameters, you may want
to create a F<.perltidyrc> file containing them.  See the perltidy man
page for an explanation.

2. Rather than specifying values for these parameters, it is probably
easier to accept the defaults and then edit a style sheet.  The style
sheet contains comments which should make this easy.

3. The syntax-colored html files can be very large, so it may be best to
split large files into smaller pieces to improve download times.

=back

=head1 SOME COMMON INPUT CONVENTIONS

=head2 Specifying Block Types

Several parameters which refer to code block types may be customized by also
specifying an associated list of block types.  The type of a block is the name
of the keyword which introduces that block, such as B<if>, B<else>, or B<sub>.
An exception is a labeled block, which has no keyword, and should be specified
with just a colon.  To specify all blocks use B<'*'>.

For example, the following parameter specifies C<sub>, labels, C<BEGIN>, and
C<END> blocks:

   -cscl="sub : BEGIN END"

(the meaning of the -cscl parameter is described above.)  Note that
quotes are required around the list of block types because of the
spaces.  For another example, the following list specifies all block types
for vertical tightness:

   -bbvtl='*'

=head2 Specifying File Extensions

Several parameters allow default file extensions to be overridden.  For
example, a backup file extension may be specified with B<-bext=ext>,
where B<ext> is some new extension.  In order to provides the user some
flexibility, the following convention is used in all cases to decide if
a leading '.' should be used.  If the extension C<ext> begins with
C<A-Z>, C<a-z>, or C<0-9>, then it will be appended to the filename with
an intermediate '.' (or perhaps an '_' on VMS systems).  Otherwise, it
will be appended directly.  

For example, suppose the file is F<somefile.pl>.  For C<-bext=old>, a '.' is
added to give F<somefile.pl.old>.  For C<-bext=.old>, no additional '.' is
added, so again the backup file is F<somefile.pl.old>.  For C<-bext=~>, then no
dot is added, and the backup file will be F<somefile.pl~>  .  

=head1 SWITCHES WHICH MAY BE NEGATED

The following list shows all short parameter names which allow a prefix
'n' to produce the negated form:

 D    anl asc  aws  b    bbb bbc bbs  bl   bli  boc bok  bol  bot  ce
 csc  dac dbc  dcsc ddf  dln dnl dop  dp   dpro dsc dsm  dsn  dtt  dwls
 dwrs dws f    fll  frm  fs  hsc html ibc  icb  icp iob  isbc lal  log
 lp   lsl ohbr okw  ola  oll opr opt  osbr otr  ple  pod  pvl  q
 sbc  sbl schb scp  scsb sct se  sfp  sfs  skp  sob sohb sop  sosb sot
 ssc  st  sts  syn  t    tac tbc toc  tp   tqw  tsc w    x    bar  kis

Equivalently, the prefix 'no' or 'no-' on the corresponding long names may be
used.

=head1 LIMITATIONS

=over 4

=item  Parsing Limitations

Perltidy should work properly on most perl scripts.  It does a lot of
self-checking, but still, it is possible that an error could be
introduced and go undetected.  Therefore, it is essential to make
careful backups and to test reformatted scripts.

The main current limitation is that perltidy does not scan modules
included with 'use' statements.  This makes it necessary to guess the
context of any bare words introduced by such modules.  Perltidy has good
guessing algorithms, but they are not infallible.  When it must guess,
it leaves a message in the log file.

If you encounter a bug, please report it.

=item  What perltidy does not parse and format

Perltidy indents but does not reformat comments and C<qw> quotes. 
Perltidy does not in any way modify the contents of here documents or
quoted text, even if they contain source code.  (You could, however,
reformat them separately).  Perltidy does not format 'format' sections
in any way.  And, of course, it does not modify pod documents.

=back

=head1 FILES

=over 4

=item Temporary files

Under the -html option with the default --pod2html flag, a temporary file is
required to pass text to Pod::Html.  Unix systems will try to use the POSIX
tmpnam() function.  Otherwise the file F<perltidy.TMP> will be temporarily
created in the current working directory.

=item Special files when standard input is used

When standard input is used, the log file, if saved, is F<perltidy.LOG>,
and any errors are written to F<perltidy.ERR> unless the B<-se> flag is
set.  These are saved in the current working directory.  

=item Files overwritten

The following file extensions are used by perltidy, and files with these
extensions may be overwritten or deleted: F<.ERR>, F<.LOG>, F<.TEE>,
and/or F<.tdy>, F<.html>, and F<.bak>, depending on the run type and
settings.

=item  Files extensions limitations

Perltidy does not operate on files for which the run could produce a file with
a duplicated file extension.  These extensions include F<.LOG>, F<.ERR>,
F<.TEE>, and perhaps F<.tdy> and F<.bak>, depending on the run type.  The
purpose of this rule is to prevent generating confusing filenames such as
F<somefile.tdy.tdy.tdy>.

=back

=head1 SEE ALSO

perlstyle(1), Perl::Tidy(3)

=head1 VERSION

This man page documents perltidy version 20160302.

=head1 CREDITS

Michael Cartmell supplied code for adaptation to VMS and helped with
v-strings.

Yves Orton supplied code for adaptation to the various versions
of Windows. 

Axel Rose supplied a patch for MacPerl.

Hugh S. Myers designed and implemented the initial Perl::Tidy module interface. 

Many others have supplied key ideas, suggestions, and bug reports;
see the CHANGES file.

=head1 AUTHOR

  Steve Hancock
  email: perltidy at users.sourceforge.net
  http://perltidy.sourceforge.net

=head1 COPYRIGHT

Copyright (c) 2000-2012 by Steve Hancock

=head1 LICENSE

This package is free software; you can redistribute it and/or modify it
under the terms of the "GNU General Public License".

Please refer to the file "COPYING" for details.

=head1 DISCLAIMER

This package is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the "GNU General Public License" for more details.

__END__
:endofperl
