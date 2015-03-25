#!/bin/sh

main() {

N=60
dir0=`dirname $0`
binary=$dir0/ls4sweep
chtimes=$dir0/chtimes

if [ ! -x $binary -o ! -x $chtimes ]; then
	echo "Error: cannot find $binary or $chtimes" 1>&2
	return 1
fi

D=`mktemp -d -t ls4sweep_test.XXXX`
trap "rm -rf $D" EXIT
echo "Running in directory $D"

for (( i = 0; i < N ; i++ )); do
#	echo Start iteration $i...
#	[ $i = 0 ] || $chtimes -$[24*60*60] $D/*.tmp
	echo $i > $D/$i.tmp
	[ $i = 0 ] || $chtimes -$[$i * 24*60*60] $D/$i.tmp
#	$binary "$@" "$D/*.tmp" 7 1 3 7 11 30
#	echo Return code = $?
done

#$binary "$@" "$D/*.tmp" 7 1 3 7 11 30
$binary "$@" '7 1 3 7 11 30' $D/*.tmp
echo Return code = $?

}

main "$@"

## EOF ##
