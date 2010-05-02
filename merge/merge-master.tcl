#
# Use this script for prepea merging celestia's source files
#
 
source merge.cfg

proc show-help {} {
    puts "Usage:"
    puts "$::argv0 COMMAND

Commands:
    checkout (co)         Checkout from svn and merge files.
    git-move              Search files that was moved in svn and 
                          move them in current git repo.
    apply                 Apply changes, you may need do it after
                          git-move
    stats \[opt\]           Statistics after checkout.
                          Where opt maybe:
      add                  list of new files
      del                  removed from svn files
      move                 moved files
    git-add               Copy some file from work/ and do git add
    copy-again            Recopy files from work
    makefile              Compare make files for including *.cpp sources
    cpp                   List of cpp files in your Makefile.am
    help                  Print help.

Usual you need do `checkout' first, than `git-move', and `apply'.
"
}

if {$::argc == 0} {
    show-help
    exit
}

proc update-stats {srcdirs} {
    foreach dir $srcdirs {
	set update_log "svn-$dir-update.log"
	puts " ===\[ $dir/ \]"
	set new   [exec sh -c "cat $update_log| grep \"^A \" | wc -l"]
	set del   [exec sh -c "cat $update_log| grep \"^D \" | wc -l"]
	set upd   [exec sh -c "cat $update_log| grep \"^U \" | wc -l"]
	set merg  [exec sh -c "cat $update_log| grep \"^G \" | wc -l"]
	set confl [exec sh -c "cat $update_log| grep \"^C \" | wc -l"]
	puts "  New files: $new"
	puts "  Deleted:   $del"
	puts "  Updated:   $upd"
	puts "  Merged:    $merg"
	puts "  Conflicts: $confl"
	puts " * See $update_log for conflicts."
    }
}
# return 1 if element in list
proc in {list element} {expr [lsearch -exact $list $element] >= 0}

proc lrmdups list {
    set res {}
    foreach element $list {
	if {[lsearch -exact $res $element]<0} {lappend res $element}
    }
    set res
}

set SEPAR "=================================="

if {[lindex $::argv 0] == "co" || 
    [lindex $::argv 0] == "checkout"} {
    #-----------------------------------------------
    # checkout
    #-----------------------------------------------
    
    catch {file delete -force work}
    file mkdir work
    
    puts $SEPAR
    
    set svn_rev [exec cat $svn_revision_file]
    puts "Checkout last revision - $svn_rev"

    cd work
    foreach chk $srcdirs {
    	puts $SEPAR
    	puts "Checkout celestia/$chk ..." 
    	puts [exec svn co http://celestia.svn.sourceforge.net/svnroot/celestia/trunk/celestia/$chk -r $svn_rev]
    }
    cd ..

    puts $SEPAR

    puts "Copy actual files to work dir"
    foreach {dir name depth} $mcopy {
    	puts "$dir/ $name"
    	set files [exec find ../$dir -maxdepth $depth -name "$name"]
    	foreach f $files {
    	    # skip "../"
    	    set f [string range $f 3 end]
    	    if {![file isdirectory ../$f]} {
    		file copy -force ../$f work/[file dirname $f]/
    	    } else {
    		catch {file mkdir work/$f}
    	    }	
    	}
    }

    puts $SEPAR
    # write diff file before updating
    puts " Your diff from last $svn_rev revision are stored into:"
    foreach dir $srcdirs {
	set diffname r$svn_rev-$dir.diff
	puts $diffname
	exec sh -c "(cd work/$dir; svn diff > ../../$diffname)"
    }
    
    puts $SEPAR
    puts " Updating and merging:"
    foreach dir $srcdirs {
	set update_log "svn-$dir-update.log"
	puts " $dir .."
	if [catch {exec sh -c "(cd work/$dir; svn up > ../../$update_log)"} msg ] {
	    puts $msg
	    puts " * You can go ot work/ dir and do `svn up` manually for"
	    puts "each sub dirs."
	    if [string match $msg *"unversioned file of the same name already exists"* ] {
		puts " * Possible some files has been moved since last merge."
	    }
	    exit
	}
    }    

    puts $SEPAR
    update-stats $srcdirs

    puts " * Run ./commit-svn.sh to apply updates."
} elseif {[lindex $::argv 0] == "git-move" } {
    #-----------------------------------------------
    # move files in git repo
    #-----------------------------------------------
    puts $SEPAR
    puts " Search files for moving"
    set add {}
    set del {}
    set files 0
    set of [open "moved-files.lst" w]
    foreach dir $srcdirs {
	set update_log "svn-$dir-update.log"
	set add {}
	puts $dir
	catch {set add [exec sh -c "cat $update_log | grep \"^A \" | sed 's/^A//'" ]}
	set del {}
	catch {set del [exec sh -c "cat $update_log | grep \"^D \" | sed 's/^D//'"]}
	foreach d $del {
	    set d $dir/$d
	    set f1 [file tail $d]
	    foreach a $add {
		set a $dir/$a
		set f2 [file tail $a]
		
		if {$f1 == $f2} {
		    puts "$f1: \t[file dirname $d] -> [file dirname $a]"
		    catch {file mkdir ../[file dirname $a]}
		    if [catch {exec git mv ../$d ../$a } msg] {
			puts $msg
		    } else {
			incr files
			puts $of "\"$dir/$a\"\t\"$dir/$d\""
		    }
		}
	    }
	}
    }
    close $of

    puts $SEPAR
    if $files {
	puts "Moved $files file(s)."
	puts "Don't forget to modify Makefile.am."
	puts "See"
    }
    puts "Done."
} elseif {[lindex $::argv 0] == "apply" } {
    #-----------------------------------------------
    # apply
    #-----------------------------------------------
    set svn_old_rev [exec cat ${svn_revision_file}]
    cd work/[lindex $srcdirs 0]
    set svn_new_rev [exec sh -c "svn info | grep Revision | awk '{print \$2}'"]

    # cd work/
    cd ../
    puts $SEPAR
    puts " Merging celestia's sources"
    puts " Old revision: $svn_old_rev"
    puts " New revision: $svn_new_rev"
    puts $SEPAR
    puts " Make sure that no there no conflicts and"
    puts  " press the \[Enter\] or \[Return\] key to continue, ^C to cancel "
    set discard [gets stdin]

    foreach {dir name depth} $mcopy {
	puts "$dir/ $name"
	set files [exec find ./$dir -maxdepth $depth -name "$name" ]
	foreach f $files {
	    puts $f
	    # skip ./
	    set f [string range $f 2 end]
	    if {![file isdirectory $f]} {
	    	file copy -force ./$f [file join ../../ [file dirname $f]]/
	    } else {
		puts "mkdir [file join ../../ $f]"
	    	catch {file mkdir [file join ../../ $f]}
	    }	
	}
    }
    exec echo $svn_new_rev > ../${svn_revision_file}
    puts "Done."
} elseif {[lindex $::argv 0] == "stats" } {
    if { $::argc == 1 } {
	update-stats $srcdirs
    } else {
	set add {}
	set del {}
	foreach dir $srcdirs {
	    set update_log "svn-$dir-update.log"
	    set add {}
	    catch {set add [exec sh -c "cat $update_log | grep \"^A \" | sed 's/^A//'" ]}
	    set del {}
	    catch {set del [exec sh -c "cat $update_log | grep \"^D \" | sed 's/^D//'"]}
	    if {[lindex $::argv 1] == "del"} {
		foreach d $del {
		    set d $dir/$d
		    set f1 [file tail $d]
		    set found no
		    foreach a $add {
			set a $dir/$a
			set f2 [file tail $a]
			if {$f1 == $f2} {
			    set found yes
			    break;
			}
		    }
		    if {!$found} {
			puts $d
		    }
		}
	    } elseif {[lindex $::argv 1] == "add"} {
		foreach a $add {
		    set a $dir/$a
		    set f1 [file tail $a]
		    set found no
		    foreach b $del {
			set b $dir/$b
			set f2 [file tail $b]
			if {$f1 == $f2} {
			    set found yes
			    break;
			}
		    }
		    if {!$found} {
			puts $a
		    }
		}
	    } elseif {[lindex $::argv 1] == "move"} {
		foreach d $del {
		    set d $dir/$d
		    set f1 [file tail $d]
		    foreach a $add {
			set a $dir/$a
			set f2 [file tail $a]
			
			if {$f1 == $f2} {
			    puts "$f1: \t[file dirname $d] -> [file dirname $a]"
			}
		    }
		}
		
	    }
	}
    }
} elseif {[lindex $::argv 0] == "git-add" } {
    if { $::argc == 1 } {
	puts "Forgot to specify file to copy?"
    } else {
	for {set i 1} {$i<=$::argc} {incr i} {
	    set file [lindex $::argv $i]
	    if {$file == ""} {
		exit
	    }
	    
	    if [file isdirectory work/$file] {
		puts "$file is directory"
		exit
	    }
	    if [file exists ../$file] {
		puts "File exists, overwrite."
	    }
	    set dir ../[file dirname $file]
	    catch {mkdir $dir}
	    file copy -force work/$file $dir
	    puts "copy work/$file $dir"
	    exec git add ../$file
	}
    }
} elseif {[lindex $::argv 0] == "copy-again" } {
    cd work
    foreach {dir name depth} $mcopy {
	puts "$dir/ $name"
	set files [exec find ./$dir -maxdepth $depth -name "$name" ]
	foreach f $files {
	    puts $f
	    # skip ./
	    if {![file isdirectory $f]} {
	    	file copy -force ./$f [file join ../../ [file dirname $f]]/
	    	puts "copy ./$f [file join ../../ [file dirname $f]]/"
	    } else {
		puts "mkdir [file join ../../ $f]"
	    	catch {file mkdir [file join ../../ $f]}
	    }	
	}
    }
} elseif {[lindex $::argv 0] == "makefile" } {
    puts " `+' - file in your makefile but not in celestia's"
    puts " `-' - file in celestia makefile but not in yours"
    set dirs {}
    foreach {dir name depth} $mcopy {
	lappend dirs $dir
    }

    set dirs [lrmdups $dirs]

    foreach dir $dirs {
	puts "==== \[ $dir/Makefile.am \]"
	set src1 {}
	catch {set src1 [exec sh -c "grep .cpp work/$dir/Makefile.am | sed -e 's/\\\\//' | sed -e 's/.*=\\(.*\\)/\\1/'"]}
	set src2 {}
	catch {set src2 [exec sh -c "grep .cpp ../$dir/Makefile.am | sed -e 's/\\\\//' | sed -e 's/.*=\\(.*\\)/\\1/'"]}
	foreach f1 $src1 {
	    if {![in $src2 $f1]} {
		puts "- $f1"
	    }	    
	}
	foreach f1 $src2 {
	    if {![in $src1 $f1]} {
		puts "+ $f1"
	    }	    
	}

    }    
} elseif {[lindex $::argv 0] == "cpp" } {
    set dirs {}
    foreach {dir name depth} $mcopy {
	lappend dirs $dir
    }

    set dirs [lrmdups $dirs]

    foreach dir $dirs {
	set src {}
	catch {set src [exec sh -c "grep .cpp ../$dir/Makefile.am | sed -e 's/\\\\//' | sed -e 's/.*=\\(.*\\)/\\1/'"]}
	foreach f $src {
	    puts $dir/$f	    
	}

    }    
}

