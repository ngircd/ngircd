# [ngIRCd](https://ngircd.barton.de) - Internet Relay Chat Server

This *Quick Start* document explains how to configure ngIRCd, the lightweight
Internet Relay Chat (IRC) server, using some "real world" scenarios.

## Simple Single-Instance Server

ngIRCd needs at least a valid IRC server name configured, therefore the
simplest configuration file looks like this:

``` ini
[Global]
Name = irc.example.net
````

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
any way and can be an arbitrary string -- but which *must* contain at least
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
