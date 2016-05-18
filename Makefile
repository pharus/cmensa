#
# Makefile for cmensa
#

include config.mk


test: test.c

dist: clean
	@mkdir cmensa
	@cp -rv types cmensa.c cmensa.h cmensa
	@echo built dist

zip: dist
	@zip -rv cmensa.zip cmensa
	@$(RM) -r cmensa
	@echo built zip 

clean:
	@echo cleaning
	@$(RM) -r cmensa cmensa.zip $(OBJS) test

