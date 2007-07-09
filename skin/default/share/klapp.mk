# export user set vars
ifdef KLONE_VERSION
export KLONE_VERSION
endif
ifdef KLONE_CONF_ARGS 
export KLONE_CONF_ARGS 
endif
ifdef MAKL_PLATFORM 
export MAKL_PLATFORM 
endif
ifdef KLONE_CUSTOM_TC 
export KLONE_CUSTOM_TC 
endif
ifdef WEBAPP_DIR 
export WEBAPP_DIR 
endif
ifdef SUBDIR 
export SUBDIR 
endif
ifdef WEBAPP_CFLAGS 
export WEBAPP_CFLAGS 
endif
ifdef WEBAPP_LDFLAGS 
export WEBAPP_LDFLAGS 
endif
ifdef WEBAPP_LDADD 
export WEBAPP_LDADD 
endif

KLONE_DIR = $(shell pwd)/klone-$(KLONE_VERSION)/
KLONE_TGZ = klone-$(KLONE_VERSION).tar.gz

all:
	[ -f $(KLONE_DIR)/configure ] || make src
	[ -f $(KLONE_DIR)/Makefile.conf ] || ( cd $(KLONE_DIR) && ./configure )
	make -C $(KLONE_DIR)
	ln -sf $(KLONE_DIR)/kloned 

clean: 
	if [ -d $(KLONE_DIR) ]; then \
		make MAKL_TC= -C $(KLONE_DIR)/build/makl toolchain ; \
		make MAKL_TC= -C $(KLONE_DIR) clean; \
		make MAKL_TC= -C $(KLONE_DIR) dist-clean; \
	fi
	rm -f kloned

src: $(KLONE_TGZ)
	tar zxvf $(KLONE_TGZ)

$(KLONE_TGZ):
	wget -c http://koanlogic.com/klone/$(KLONE_TGZ)


