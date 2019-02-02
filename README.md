Introduction
-------------------------------------------------------------------------------

ClipIt is a lightweight GTK+ clipboard manager.

Project website: https://github.com/CristianHenzel/ClipIt


ClipIt - Lightweight GTK+ Clipboard Manager
-------------------------------------------------------------------------------

Copyright (C) 2010-2019 by Cristian Henzel <oss@rspwn.com>

Copyright (C) 2011 by Eugene Nikolsky <pluton.od@gmail.com>



forked from parcellite, which is

Copyright (C) 2007-2008 Gilberto "Xyhthyx" Miralla <xyhthyx@gmail.com>


How to compile and install clipit
-------------------------------------------------------------------------------

Requirements:
* gtk+ >= 2.10.0 (>= 3.0 for gtk+3)
* xdotool - for automatic paste functionality

Download the clipit source code, then:

    $ tar zxvf clipit-x.y.z.tar.gz
    $ cd clipit-x.y.z
    $ ./autogen.sh
    $ ./configure
    $ make
    $ sudo make install
