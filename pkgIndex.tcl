# -*- tcl -*-
# Tcl package index file, version 1.1
#
if {[package vsatisfies [package provide Tcl] 9.0-]} {
    package ifneeded stbimage 1.0 \
	    [list load [file join $dir libtcl9stbimage1.0.so] [string totitle stbimage]]
} else {
    package ifneeded stbimage 1.0 \
	    [list load [file join $dir libstbimage1.0.so] [string totitle stbimage]]
}
