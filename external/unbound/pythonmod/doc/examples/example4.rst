DNS-based language dictionary
===============================

This example shows how to create a simple language dictionary based on **DNS**
service within 15 minutes. The translation will be performed using TXT resource records.

Key parts
-----------

Initialization
~~~~~~~~~~~~~~~~~~~~~~~
On **init()** module loads dictionary from a text file containing records in ``word [tab] translation`` format.
::

   def init(id, cfg):
      log_info("pythonmod: dict init")
      f = open("examples/dict_data.txt", "r")
      ...

The suitable file can be found at http://slovnik.zcu.cz

DNS query and word lookup
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Let's define the following format od DNS queries: ``word1[.]word2[.] ... wordN[.]{en,cs}[._dict_.cz.]``.
Word lookup is done by simple ``dict`` lookup from broken DNS request.
Query name is divided into a list of labels. This list is accesible as qname_list attribute.
::

   aword = ' '.join(qstate.qinfo.qname_list[0:-4]) #skip last four labels
   adict = qstate.qinfo.qname_list[-4] #get 4th label from the end

   words = [] #list of words
   if (adict == "en") and (aword in en_dict):
      words = en_dict[aword] 

   if (adict == "cs") and (aword in cz_dict):
      words = cz_dict[aword] # CS -> EN

In the first step, we get a string in the form: ``word1[space]word2[space]...word[space]``.
In the second assignment, fourth label from the end is obtained. This label should contains *"cs"* or *"en"*.
This label determines the direction of translation.


Forming of a DNS reply
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DNS reply is formed only on valid match and added as TXT answer.
::

	msg = DNSMessage(qstate.qinfo.qname_str, RR_TYPE_TXT, RR_CLASS_IN, PKT_AA)

	for w in words:
		msg.answer.append("%s 300 IN TXT \"%s\"" % (qstate.qinfo.qname_str, w.replace("\"", "\\\"")))

	if not msg.set_return_msg(qstate):
		qstate.ext_state[id] = MODULE_ERROR 
		return True

	qstate.return_rcode = RCODE_NOERROR
	qstate.ext_state[id] = MODULE_FINISHED 
	return True

In the first step, a :class:`DNSMessage` instance is created for a given query *(type TXT)*.
The fourth argument specifies the flags *(authoritative answer)*.
In the second step, we append TXT records containing the translation *(on the right side of RR)*.
Then, the response is finished and ``qstate.return_msg`` contains new response.
If no error, the module sets :attr:`module_qstate.return_rcode` and :attr:`module_qstate.ext_state`.

**Steps:**

1. create :class:`DNSMessage` instance
2. append TXT records containing the translation
3. set response to ``qstate.return_msg``

Testing
-------

Run the Unbound server:

``root@localhost>unbound -dv -c ./test-dict.conf``

In case you use own configuration file, don't forget to enable Python module::

	module-config: "validator python iterator"

and use valid script path::

	python-script: "./examples/dict.py"

The translation from english word *"a bar fly"* to Czech can be done by doing:

``>>>dig TXT @127.0.0.1 a.bar.fly.en._dict_.cz``

::	

	; (1 server found)
	;; global options:  printcmd
	;; Got answer:
	;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 48691
	;; flags: aa rd ra; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 0
	
	;; QUESTION SECTION:
	;a.bar.fly.en._dict_.cz.		IN	TXT
	
	;; ANSWER SECTION:
	a.bar.fly.en._dict_.cz.	300	IN	TXT	"barov\253 povale\232"
	
	;; Query time: 5 msec
	;; SERVER: 127.0.0.1#53(127.0.0.1)
	;; WHEN: Mon Jan 01 17:44:18 2009
	;; MSG SIZE  rcvd: 67
	
``>>>dig TXT @127.0.0.1 nic.cs._dict_.cz``
::
	
	; <<>> DiG 9.5.0-P2 <<>> TXT @127.0.0.1 nic.cs._dict_.cz
	; (1 server found)
	;; global options:  printcmd
	;; Got answer:
	;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 58710
	;; flags: aa rd ra; QUERY: 1, ANSWER: 6, AUTHORITY: 0, ADDITIONAL: 0
	
	;; QUESTION SECTION:
	;nic.cs._dict_.cz.		IN	TXT
	
	;; ANSWER SECTION:
	nic.cs._dict_.cz.	300	IN	TXT	"aught"
	nic.cs._dict_.cz.	300	IN	TXT	"naught"
	nic.cs._dict_.cz.	300	IN	TXT	"nihil"
	nic.cs._dict_.cz.	300	IN	TXT	"nix"
	nic.cs._dict_.cz.	300	IN	TXT	"nothing"
	nic.cs._dict_.cz.	300	IN	TXT	"zilch"
	
	;; Query time: 0 msec
	;; SERVER: 127.0.0.1#53(127.0.0.1)
	;; WHEN: Mon Jan 01 17:45:39 2009
	;; MSG SIZE  rcvd: 143

Proof that the unbound still works as resolver.

``>>>dig A @127.0.0.1 www.nic.cz``
::

	; (1 server found)
	;; global options:  printcmd
	;; Got answer:
	;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 19996
	;; flags: qr rd ra; QUERY: 1, ANSWER: 1, AUTHORITY: 3, ADDITIONAL: 5
	
	;; QUESTION SECTION:
	;www.nic.cz.			IN	A
	
	;; ANSWER SECTION:
	www.nic.cz.		1662	IN	A	217.31.205.50
	
	;; AUTHORITY SECTION:
	...

Complete source code
--------------------

.. literalinclude:: ../../examples/dict.py
   :language: python
