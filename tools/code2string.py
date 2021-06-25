# Use this to minify the code first:
# https://javascript-minifier.com/

import sys

if __name__ == "__main__":
	if len(sys.argv) < 2:
		print("Please specify a file to process.")
		sys.exit(1)
	
	with open(sys.argv[1], "r") as f:
		lines = f.read().splitlines()
		output = "const char *my_string = "
		for line in lines:
			line = line.replace("\\", "\\\\")
			line = line.replace("\"", "\\\"")
			output += "\n    \"" + line + "\\n\""
		output += ";\n"
		print(output)
