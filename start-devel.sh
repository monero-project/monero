project=Monero
variant=$1
echo "Starting developer version of $project, variant=$variant"

if [[ "$DEVELOPER_LOCAL_TOOLS" == 1 ]] ; then
	echo "Using LOCAL TOOLS, will export needed local libs (like boost etc)"
	export LD_LIBRARY_PATH="$HOME/.local/lib:$HOME/.local/lib64"
else
	echo "Not using local tools, will use global libraries"
fi

./build/$variant/src/bitmonerod --force-fast-exit --no-igd 

