# Example configuration file for edns.py
server:
	verbosity: 1
	interface: 0.0.0.0
	do-daemonize: no
	access-control: 0.0.0.0/0 allow
	chroot: ""
	username: ""
	directory: ""
	logfile: ""
	pidfile: "unbound.pid"
	module-config: "validator python iterator"

# Python config section
python:
	# Script file to load
	python-script: "./examples/inplace_callbacks.py"
