/** \file      sbt_common.v
 *  \brief     Shared-library exports and version control
 *  \copyright Copyright 2015 Silver Bullet Technology
 *
 *             Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *             use this file except in compliance with the License.  You may obtain a copy
 *             of the License at: 
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *             Unless required by applicable law or agreed to in writing, software
 *             distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *             WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 *             License for the specific language governing permissions and limitations
 *             under the License.
 *
 * vim:ts=4:noexpandtab
 */
sbt_common_0.1 {
	global:
		/* src/lib/buffer.c */
		buffer_alloc;
		buffer_reset;
		buffer_fmt;
		buffer_cpy;
		buffer_set;
		buffer_read;
		buffer_discard;
		buffer_write;
		buffer_chr;
		/* src/lib/child.c */
		child_spawn;
		child_argv;
		child_check;
		child_close;
		child_fd_set;
		child_select;
		child_fd_is_set;
		child_can_read;
		child_read;
		child_can_write;
		child_write;
		/* src/lib/clocks.c */
		get_clocks;
		clocks_to_us;
		clocks_to_ms;
		clocks_to_s;
		us_to_clocks;
		ms_to_clocks;
		s_to_clocks;
		clocks_to_tv;
		tv_to_clocks;
		/* src/lib/descript.c */
		descript_value_for_label;
		descript_label_for_value;
		__descript_dump_labels;
		/* src/lib/genlist.c */
		genlist_alloc;
		genlist_free;
		genlist_destroy;
		genlist_reset;
		genlist_prepend;
		genlist_append;
		genlist_head;
		genlist_tail;
		genlist_insert;
		genlist_remove;
		genlist_sort;
		genlist_search;
		genlist_next;
		/* src/lib/growlist.c */
		growlist_alloc;
		growlist_init;
		growlist_copy;
		growlist_grow;
		growlist_done;
		growlist_empty;
		growlist_swap;
		growlist_reset;
		growlist_append;
		growlist_pop;
		growlist_insert;
		growlist_remove;
		growlist_sort;
		growlist_search;
		growlist_next;
		growlist_intersect;
		/* src/lib/log.c */
		log_fopen;
		log_dupe;
		log_openlog;
		log_trace;
		log_close;
		log_config;
		log_hexdump_line;
		log_hexdump;
		log_printf;
		_log_set_global_level;
		_log_set_module_level;
		log_get_module_list;
		log_label_for_level;
		log_level_for_label;
		log_call_enter;
		log_call_leave;
		log_call_trace;
		/* src/lib/mbuf.c */
		_mbuf_alloc;
		_mbuf_ref;
		_mbuf_deref;
		_mbuf_free;
		_mbuf_clone;
		mbuf_beg_get;
		mbuf_beg_set;
		mbuf_end_get;
		mbuf_end_set;
		mbuf_cur_get;
		mbuf_cur_set;
		mbuf_beg_ptr;
		mbuf_end_ptr;
		mbuf_cur_ptr;
		mbuf_max_ptr;
		mbuf_get_avail;
		mbuf_get_8;
		mbuf_get_n16;
		mbuf_get_be16;
		mbuf_get_le16;
		mbuf_get_n32;
		mbuf_get_be32;
		mbuf_get_le32;
		mbuf_get_n64;
		mbuf_get_be64;
		mbuf_get_le64;
		mbuf_get_mem;
		mbuf_set_avail;
		mbuf_set_8;
		mbuf_set_n16;
		mbuf_set_be16;
		mbuf_set_le16;
		mbuf_set_n32;
		mbuf_set_be32;
		mbuf_set_le32;
		mbuf_set_n64;
		mbuf_set_be64;
		mbuf_set_le64;
		mbuf_set_mem;
		mbuf_printf;
		mbuf_fill;
		mbuf_read;
		mbuf_write;
		_mbuf_dump;
		/* src/lib/mqueue.c */
		mqueue_alloc;
		mqueue_init;
		mqueue_done;
		mqueue_free;
		mqueue_flush;
		mqueue_enqueue;
		mqueue_dequeue;
		/* src/lib/packlist.c */
		packlist_size;
		packlist_alloc;
		packlist_data;
		/* src/lib/timer.c */
		timer_alloc;
		_timer_init;
		_timer_is_attached;
		_timer_detach;
		_timer_attach;
		timer_check;
		timer_head_due_at;
		timer_head_due_in;
		timer_head_due_now;
		timer_head_call;
		/* src/lib/util.c */
		macfmt;
		dump_line;
		dump_buff;
		strmatch;
		binval;
		pow10_32;
		pow10_64;
		dec_to_u_fix;
		dec_to_s_fix;
		path_match;
		proc_write;
		set_fd_bits;
		str_dup_sprintf;
		/* src/lib/.c */
		uuid_to_str;
		uuid_from_str;
		uuid_random;
	local:
		*;
};
 
