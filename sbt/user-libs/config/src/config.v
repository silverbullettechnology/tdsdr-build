/* File    : src/config.v
 * Project : Config parser library
 * Descript: public exports and version control
 * License : Copyright (C) 2010,2011 Morgan Hughes <morgan.hughes@silver-bullet-tech.com>
 * vim:ts=4:noexpandtab
 */
config_0.1 {
	global:
		/* src/config_buffer.c */
		config_buffer_init;
		config_buffer_alloc;
		config_buffer_grow;
		config_buffer_done;
		config_buffer_free;
		config_buffer_read;
		config_buffer_discard;
		config_buffer_write;
		/* src/config_parse.c */
		config_parse;
	local:
		*;
};
