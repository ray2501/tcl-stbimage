tcl-stbimage
=====

[stb](https://github.com/nothings/stb) is a single-file public domain 
(or MIT licensed) libraries for C/C++.

It is a Tcl extension for stb_image. This package is using stb_image Easy-to-use
API to load (ex. jpg/png/tga/bmp), resize and write (ex. jpg/png/tga/bmp) image.


Implement commands
=====

::stbimage::load filename   
::stbimage::load_from_memory filedata   
::stbimage::resize inputdata srcwidth srcheight dstwitdh dstheight num_channels  
::stbimage::write format filename width height channels data  
::stbimage::rgb2rgba inputdata width height  

`::stbimage::rgb2rgba` is a help command, try to convert RGB image to RGBA
image.

`format` value should be -
jpg, png, tga, bmp


UNIX BUILD
=====

Building under most UNIX systems is easy, just run the configure script
and then run make. For more information about the build process, see
the tcl/unix/README file in the Tcl src dist. The following minimal
example will install the extension in the /opt/tcl directory.

    $ cd tcl-stbimage
    $ ./configure --prefix=/opt/tcl
    $ make
    $ make install
	
If you need setup directory containing tcl configuration (tclConfig.sh),
below is an example:

    $ cd tcl-stbimage
    $ ./configure --with-tcl=/opt/activetcl/lib
    $ make
    $ make install


Example
=====

	package require stbimage

	set d [::stbimage::load test.jpg]
	set width [dict get $d width]
	set height [dict get $d height]
	set channel [dict get $d channel]
	set data [dict get $d data]
	set neww [expr [dict get $d width] * 2]
	set newh [expr [dict get $d height] * 2]
	set d2 [::stbimage::resize $data $width $height $neww $newh $channel]
	set newdata [dict get $d2 data]
	::stbimage::write jpg test2.jpg $neww $newh $channel $newdata
	::stbimage::write png test2.png $neww $newh $channel $newdata
	::stbimage::write tga test2.tga $neww $newh $channel $newdata
	::stbimage::write bmp test2.bmp $neww $newh $channel $newdata

