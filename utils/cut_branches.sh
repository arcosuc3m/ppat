#!/bin/bash
callgraph="callgraph.dot"
output="out.dot"

# Remove duplicate lines
awk '!a[$0]++' $callgraph > temp
mv temp $callgraph

# Only nodes
grep '\[' $callgraph | sed 's/^\s*\| \[.*//g' > temp
grep -e '->' $callgraph | sed 's/^\s*\| ->.*//g' >> temp
grep -e '->' $callgraph | sed 's/^.*-> \|;.*//g' >> temp
sort temp | uniq > nodes.txt

# Nodes and direction
grep -e "->" $callgraph | sed 's/^\s*\|;.*//g' | sed 's/ ->//g' > edges.txt

# Determine main node
main=`grep -e "main" $callgraph | sed 's/^\s*\| \[.*//g'`

# Determine external node
external=`grep 'external node' $callgraph | sed 's/^\s*\| \[.*//g'`

# Determine which node are not accesible from main
python useful_nodes.py "$main" > toremove.txt

# Remove nodes.txt and edges.txt
rm nodes.txt
rm edges.txt
rm temp

# get conexion external node main in order to add again later
ext_node=`grep 'external node' $callgraph`
ext_main=`grep "$external -> $main" $callgraph`

# Remove useless node from the callgraph
grep -vwF -f toremove.txt callgraph.dot > temp
pattern="/label=\"Call graph\";/a  \\$ext_node\n$ext_main"

sed -i "$pattern" temp

# toremove.txt no needed anymore
rm toremove.txt

# Demangle function names
cat temp | c++filt > temp_
mv temp_ temp

# remove std functions
grep "{std::.*(" temp | sed 's/^\s*\| \[.*//g' > std.txt
grep -vwF -f std.txt temp > $output
rm temp
rm std.txt
