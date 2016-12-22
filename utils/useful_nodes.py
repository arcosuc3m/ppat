import sys

MAX_DEPTH = float('Inf') # Number of levels to expand
#MAX_DEPTH = 1

nodes = {}
open_nodes = {}
closed_nodes = []

if len(sys.argv) != 2:
  print "USAGE: python2.7 useful_nodes.py root_node"
  sys.exit()

root_node = sys.argv[1]

with open('nodes.txt', 'r') as infile:
  lines = infile.read().splitlines()
  for line in lines:
    nodes[line] = []

with open('edges.txt', 'r') as infile:
  lines = infile.read().splitlines()
  for line in lines:
    orig, dest  = line.split()
    nodes[orig].append(dest)

# start searching in root node
open_nodes[root_node] = nodes.pop(root_node)
num_open_nodes = 1

level = MAX_DEPTH

while num_open_nodes > 0 and level > 0:
  orig, dests = open_nodes.popitem()
  for dest in dests:
    temp = nodes.pop(dest,None)
    if (temp != None):
      open_nodes[dest] = temp
  closed_nodes.append(orig)
  num_open_nodes = len(open_nodes)
  level -= 1

#for node in closed_nodes:
#  print node

for orig,_ in nodes.iteritems():
  print orig
