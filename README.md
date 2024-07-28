# FChat protocol plugin for libpurple (Pidgin)

The [FChat](http://www.kilievich.com/fchat/) is one of the famous in past peer-to-peer (p2p) chat programs for local networks.
Original FChat had a support of IRC protocol and itself FChat protocol which is lightweight and based on UDP datagrams.
This easily allowed having a chat with more than 500 peoples in the LAN network on my ISP and without any dedicated server! 
While the FChat is not developed anymore and the last version was released in 2004 it still rocks and can be customized in any aspect.

I created this plugin for the [Pidgin](https://pidgin.im/):

![](/screenshot.png)

In fact this is a plugin for [libpurple](https://developer.pidgin.im/wiki/WhatIsLibpurple) i.e. a core library for several messengers: Pidgin, Finch, [Adium](https://adium.im/), [Instantbird](http://www.instantbird.com/) and [Telepathy](https://telepathy.freedesktop.org/) with haze.


## Features
* No need for any dedicated server: just run it and start a conversation. Nobody can ban you, nobody can read your messages.
* One Main Chat Room.
* Private messages.
* You can send a beep signal to urgently ask for attention.
* There is not any roaster with contacts, but you can ask a buddy to give its own contact list.  
* Message Board: you can pin a message for everyone like "I'm selling notebook".
* Rich user info
* Custom statuses are partially supported.

## Install
Currently, it's supported only Linux and tested on Ubuntu Mate.
Since this is a plugin then you need to install IM client. For example to install Pidgin:

    sudo apt install pidgin

### Ubuntu
The plugin can be installed in Ubuntu via [PPA](https://code.launchpad.net/~stokito/+archive/ubuntu/pidgin-fchat):

    sudo add-apt-repository ppa:stokito/pidgin-fchat
    sudo apt-get update
    sudo apt install pidgin-fchat

### Build and install from sources
For other distros you can try to install from sources:

    git clone https://github.com/stokito/pidgin-fchat.git
    cd pidgin-fchat
    sudo apt install libpurple-dev
    make
    sudo make install

#### Build deb package
To build a Debian package:

    debuild

And check the parent folder for the built `*.deb` files like `pidgin-fchat_1.0.2_amd64.deb`


## Usage

* Open Pidgin, main menu Accounts / Add.
* Select protocol: ![](/share/pixmaps/pidgin/protocols/16/fchat.png) `FChat`
* Username: `0.0.0.0`
* Local Alias: `Your Name` - it will be shown to your contacts.

FChat is p2p chat program for local networks based on UDP protocol.
Your IP will be used as `Username`. If you have several network interfaces you can use different IP for them.
But most probably you want to use FChat for all networks so enter IP `0.0.0.0`.

### Encoding problems for non-English speakers
This plugin uses `UTF-8` encoding but if you are going to talk with original FChat users,
then you also have to select an encoding of your country.
For example, for Russian/Ukrainian users you have to use the charset `CP1251`.
To configure encoding Account settings and `Advanced` tab.

### Manually add a buddy 
After login FChat will send a broadcast message to connect to all users in LAN. But sometimes this may not work.
If so you can manually add a buddy by its IP address: in the main menu click on `Buddies` then `Add buddy...`:
* `Buddy's username` - input here IP address of the buddy.
* `Alias` - input here a name or nickname of the buddy.

### Join a chat
The FChat naturally supports a chat room for all users but in the Pidgin you have to "join" it yourself.
To do that in the main menu click `Buddies` then `Join a Chat...` and `Chat Room` will be `Main` so just press `Join`. 

## Support
In case of any problems or propositions please leave a ticket on [issues](https://github.com/stokito/pidgin-fchat/issues)

Hope you'll enjoy it :) Happy chatting!
