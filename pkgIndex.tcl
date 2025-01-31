# -*- tcl -*-
# Tcl package index file, version 1.1
#
if {[package vsatisfies [package provide Tcl] 9.0-]} {
    package ifneeded stbimage 1.3 \
	    [list load [file join $dir libtcl9stbimage1.3.so] [string totitle stbimage]]
} else {
    package ifneeded stbimage 1.3 \
	    [list load [file join $dir libstbimage1.3.so] [string totitle stbimage]]
}
