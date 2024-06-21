# Example: bash sym2imp.sh symbols.txt

#  | grep -v '"Instruction Label"' \
#  | grep -v '"Analysis"' \

pat='\([^,]*\),\([^,]*\),\([^,]*\),\([^,]*\),\([^,]*\),\([^,]*\),\(.*\)'
rep='\1 \2 \3'

cat "${1}" \
  | grep '"Global"' \
  | grep -v 'switchD_' \
  | sed "s/${pat}/${rep}/" \
  | sed 's/"Data Label"/l/' \
  | sed 's/"Instruction Label"/l/' \
  | sed 's/"Function"/f/' \
  | sed 's/"Thunk Function"/f/' \
  | sed 's/"//g'

#  | sed 's/\([^,]*\),\([^,]*\),\([^,]*\),\([^,]*\),\([^,]*\),\([^,]*\),\(.*\)/\1/'
