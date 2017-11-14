#need to be removed as part of make clean
COMPONENT_EXTRA_CLEAN := certs.h

aws_iot_device.o: certs.h

certs.h: $(COMPONENT_PATH)/certs/aws-root-ca.pem $(COMPONENT_PATH)/certs/bedjet.cert.pem $(COMPONENT_PATH)/certs/bedjet.private.key
	$(Q) rm -f certs.h
	$(Q) echo "const char *rootCA =" >> certs.h
	$(Q) cat $(COMPONENT_PATH)/certs/aws-root-ca.pem | tr -d "\r" | awk '{ print "\""$$0"\\r\\n\""}' >> certs.h
	$(Q) echo ";" >> certs.h
	$(Q) echo "const char *devicePrivateKey =" >> certs.h
	$(Q) cat $(COMPONENT_PATH)/certs/bedjet.private.key | tr -d "\r" | awk '{ print "\""$$0"\\r\\n\""}' >> certs.h
	$(Q) echo ";" >> certs.h
	$(Q) echo "const char *deviceCert =" >> certs.h
	$(Q) cat $(COMPONENT_PATH)/certs/bedjet.cert.pem | tr -d "\r" | awk '{ print "\""$$0"\\r\\n\""}' >> certs.h
	$(Q) echo ";" >> certs.h
