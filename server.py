from http.server import HTTPServer, SimpleHTTPRequestHandler
import sys

DEFAULT_PORT = 8000

class CORSRequestHandler(SimpleHTTPRequestHandler):
	def end_headers(self):
		self.send_header('Access-Control-Allow-Origin', '*')
		self.send_header('Access-Control-Allow-Methods', 'GET')
		self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')
		return SimpleHTTPRequestHandler.end_headers(self)

if __name__ == '__main__':
	port = 8000
	if (len(sys.argv) > 1):
		port = int(sys.argv[1])

	httpd = HTTPServer(("", port), CORSRequestHandler)
	print(f"Serving HTTP on {httpd.server_name} port {httpd.server_port}")
	httpd.serve_forever()
