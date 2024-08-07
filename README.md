tcl-stbimage
=====

[stb](https://github.com/nothings/stb) is a single-file public domain 
(or MIT licensed) libraries for C/C++.

It is a Tcl extension for stb_image. This package is using stb_image Easy-to-use
API to load (ex. jpg/png/tga/bmp), resize and write (ex. jpg/png/tga/bmp) image.


Implement commands
=====

::stbimage::load filename   
::stbimage::load_from_memory data  
::stbimage::load_from_chan chan  
::stbimage::resize inputdata srcwidth srcheight dstwitdh dstheight channels  
::stbimage::resize inputdict dstwitdh dstheight  
::stbimage::write format filename width height channels data  
::stbimage::write format filename inputdict  
::stbimage::write_to_chan format chan width height channels data  
::stbimage::write_to_chan format chan inputdict  
::stbimage::rgb2rgba inputdata width height  
::stbimage::rgb2rgba inputdict  
::stbimage::rgb2grey inputdata width height  
::stbimage::rgb2grey inputdict  
::stbimage::ascii_art inputdata srcwidth srcheight dstwitdh dstheight channels ?indentstring?  
::stbimage::ascii_art inputdict dstwitdh dstheight ?indentstring?  
::stbimage::crop inputdata srcwidth srcheight startcolumn startrow dstwitdh dstheight channels  
::stbimage::crop inputdict startcolumn startrow dstwitdh dstheight  
::stbimage::mirror inputdata width height channels flipx flipy  
::stbimage::mirror inputdict flipx flipy  
::stbimage::rotate inputdata width height channels angle  
::stbimage::rotate inputdict angle  
::stbimage::put dstdata dstwidth dstheight dstchannels srcdata srcwidth srcheight srcchannels dstcolumn dstrow ?alpha width height?  
::stbimage::put dstdict srcdict dstcolumn dstrow ?alpha width height?  

`::stbimage::rgb2rgba` is a help command, try to convert RGB image to RGBA
image.
`::stbimage::rgb2grey` is a helper command to convert RGB image to greyscale.

`format` value should be -
jpg, png, tga, bmp

An `inputdict` must contain the items `width`, `height`, `channels`,
and `data`, where the latter is a bytearray with pixel data.

The `mirror` function returns a new image which is flipped around the
X and/or Y axis.

The `rotate` function returns a new image which is rotated by 0, 90, 180,
or 270 degrees (counter clock-wise).

The `put` function returns a new image which is the source image
blended over the destination image with blend factor `alpha`. The
target position in destination is given by `dstcolumn` and `dstrow`.
If `width` and `height` are given they specify the region in
the destination image. The source image is tiled accordingly to
this region.


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
	set channels [dict get $d channels]
	set data [dict get $d data]
	set neww [expr [dict get $d width] * 2]
	set newh [expr [dict get $d height] * 2]
	set d2 [::stbimage::resize $data $width $height $neww $newh $channels]
	set newdata [dict get $d2 data]
	::stbimage::write jpg test2.jpg $neww $newh $channels $newdata
	::stbimage::write png test2.png $neww $newh $channels $newdata
	::stbimage::write tga test2.tga $neww $newh $channels $newdata
	::stbimage::write bmp test2.bmp $neww $newh $channels $newdata
	
	# flip along X
	set d3 [::stbimage::mirror $d2 1 0]
	
	# rotate by 90 deg
	set d4 [::stbimage::rotate $d2 90]
	
	# tint with pink
	set pink [dict create data [binary decode hex FFBFBF] \
		width 1 height 1 channels 3]
	set d5 [::stbimage::put $d2 $pink 0 0 64 $neww $newh]

