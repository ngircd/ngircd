# [ngIRCd](https://ngircd.barton.de) - Quick Start

This *Quick Start* document explains how to configure ngIRCd, the lightweight
Internet Relay Chat (IRC) server, using some "real world" scenarios.

## Introduction

The ngIRCd daemon can be run without any configuration file using built-in
defaults. These defaults are probably sufficient for very simple single-node
setups, but most probably need further tweaking for more "advanced" setups.

You can check the current settings by running `ngircd --configtest`. This
command not only shows the settings, it shows error, warning and hints, if it
detects any.

Therefore it is definitely best practice to *always run this check* after
making any changes to the configuration file(s) and double-check that
everything was parsed as expected!

### Configuration File and Drop-in Directory

After installing ngIRCd, a sample configuration file should have been set up if
none existed already. By default, when installing from sources, the file is
named `/usr/local/etc/ngircd.conf` (other common names, especially for
distribution packages, are `/etc/ngircd.conf` or `/etc/ngircd/ngircd.conf`).
Run the command `ngircd --configtest` to check the name of the configuration
file which is used by default on your local system.

In addition, ngIRCd supports configuration file snippets in a "drop-in"
directory which is configured with the `IncludeDir` variable in the `[Options]`
section and has a built-in default value (like `/etc/ngircd/ngircd.conf.d/`).
All configuration files matching the `*.conf` pattern are read-in from this
directory after the main `ngircd.conf` file.

It is a good idea to not edit the default `ngircd.conf` file but to create one
ore more new files in this include directory, overriding the defaults as
needed. This way you don't get any clashes when updating ngIRCd to newer
releases.

You can find the template of the sample configuration file in the `doc/`
directory as `sample-ngircd.conf` and
[online](https://ngircd.barton.de/doc/sample-ngircd.conf) on the homepage. It
contains all available options.

## Configuration File Syntax

The configuration consists of sections and parameters.

A section begins with the name of the section in square brackets (like
`[Example]`) and continues until the next section begins. Sections contain
parameters of the form `name = value`.

Section and parameter names are not case sensitive.

Please see the `ngircd.conf`(5) manual page for an in-depth description of the
configuration file, its syntax and all supported configuration options.

The sample configuration file uses comments beginning with `#` *or* `;` -- this
is only for the better understanding of the file, both comment styles are
equal. The lines commented out with `;` show example or default settings,
whereas the lines using `#` are descriptions of the options.

## Simple Single-Instance Server

A good starting point is to configure a valid (and unique!) IRC server name
(which is *not* related to a host name, it is purely a unique *server ID* that
must contain at least one dot ".").

This looks like this:

``` ini
[Global]
Name = my.irc.server
```

This results in the following *warning* in the logs when starting the daemon:
`No administrative information configured but required by RFC!` -- which works,
but is a bit ugly. So let's fix that by adding some *admin info*:

``` ini
[Global]
Name = irc.example.net
AdminInfo1 = Example IRC Server
AdminInfo2 = Anywhere On Earth
AdminEMail = admin@irc.example.net
```

*Please Note*: The server `Name` looks like a DNS host name, but it is not: in
fact it is not related to your server's fully qualified domain name (FQDN) in
any way and can be an arbitrary string -- but it *must* contain at least
one dot (".") character!

## Add a Local IRC Operator

Some IRC commands, like `REHASH` which reloads the server configuration on the
fly, require the user to authenticate to the daemon to become an *IRC
Operator* first.

So let's configure an *Operator* account in the configuration file (in
addition to what we configured above):

``` ini
[Operator]
# ID of the operator (may be different of the nickname)
Name = BigOp
# Password of the IRC operator
Password = secret
# Optional Mask from which /OPER will be accepted
;Mask = *!ident@somewhere.example.com
```

Now you can use the IRC command `OPER BigOp secret` to get *IRC Operator*
status on that server.

Please choose a sensible password, and keep in mind that the *name* is not
related to the *nickname* used by the user at all!

We don't make use of the `Mask` setting in the example above (commented out
with the `;` character), but it is a good idea to enable it whenever possible!

And you can have as many *Operator blocks* as you like, configuring multiple
different IRC Operators.

## Configuring SSL/TLS Encryption

Please see the file `SSL.md` for details.
