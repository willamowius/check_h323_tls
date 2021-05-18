#
# Makefile for check_h323_tls
#

PROG		= check_h323_tls
SOURCES		:= check_h323_tls.cxx

ifndef OPENH323DIR
OPENH323DIR=$(CURDIR)/../..
endif

STDCCFLAGS += -Wno-unused-variable

include $(OPENH323DIR)/openh323u.mak

