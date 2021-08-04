# -*- tcl -*-
# Tcl package index file, version 1.1
#
if {[package vsatisfies [package provide Tcl] 9.0-]} {
    package ifneeded stbimage 0.2 \
	    [list load [file join $dir libtcl9stbimage0.2.so] [string totitle stbimage]]
} else {
    package ifneeded stbimage 0.2 \
	    [list load [file join $dir libstbimage0.2.so] [string totitle stbimage]]
}
