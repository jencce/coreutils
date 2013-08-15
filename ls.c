#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <dirent.h>
#include <stdlib.h>
#include <libgen.h>
#include <locale.h>
#include <termios.h>

#define _GNU_SOURCE

int max_filesize;
int max_links;

struct ls_param {
	int all;
	int long_list;
	int recursive;
	int dot;
};

void init_param(struct ls_param *params)
{
	params->all = 0;
	params->long_list = 0;
	params->recursive = 0;
	params->dot = 0;
}

int parse_params(int argc, char **argv, struct ls_param *params)
{
	int c;

	struct option longopts[] = {
		{ "all",		no_argument,		0, 'a'	},
		{ "almost-all",		no_argument,		0, 'A'	},
		{ "author",		no_argument,		0, 0	},
		{ "escape",		no_argument,		0, 'b'	},
		{ "block-size",		required_argument,	0, 0	},
		{ "ignore-backups",	no_argument,		0, 'B'	},
		{ "color",		optional_argument,	0, 0	},
		{ "directory",		no_argument,		0, 'd'	},
		{ "dired",		no_argument,		0, 'D'	},
		{ "classify",		no_argument,		0, 'F'	},
		{ "file-type",		no_argument,		0, 0	},
		{ "format",		required_argument,	0, 0	},
		{ "full-time",		no_argument,		0, 0	},
		{ "group-dir-first",	no_argument,		0, 0	},
		{ "no-group",		no_argument,		0, 'G'	},
		{ "human-readable",	no_argument,		0, 'h'	},
		{ "si",			no_argument,		0, 0	},
		{ "derefrence-cmdline",	no_argument,		0, 'H'	},
		{ "hide",		required_argument,	0, 0	},
		{ "indicator-style",	required_argument,	0, 0	},
		{ "inode",		no_argument,		0, 'i'	},
		{ "ignore",		required_argument,	0, 'I'	},
		{ "derefrence",		no_argument,		0, 'L'	},
		{ "numeric-ids",	no_argument,		0, 'n'	},
		{ "literal",		no_argument,		0, 'N'	},
		{ "hide-ctl-ch",	no_argument,		0, 'q'	},
		{ "show-ctl-ch",	no_argument,		0, 0	},
		{ "quote-name",		no_argument,		0, 'Q'	},
		{ "quoting-style",	required_argument,	0, 0	},
		{ "reverse",		no_argument,		0, 'r'	},
		{ "recursive",		no_argument,		0, 'R'	},
		{ "size",		no_argument,		0, 's'	},
		{ "sort",		required_argument,	0, 0	},
		{ "time",		required_argument,	0, 0	},
		{ "time-style",		required_argument,	0, 0	},
		{ "tabsize",		required_argument,	0, 'T'	},
		{ "width",		required_argument,	0, 'w'	},
		{ "content",		no_argument,		0, 'Z'	},
		{ "version",		no_argument,		0, 0	},
		{ "help",		no_argument,		0, 0	},
		{ 0,		0,			0, 0	}
	};

	char optstring[] = "aAbcCdDfFgGhHi:I:klmopqQSrRstT:uUvw:xXZ1";

	while (1) {
		c = getopt_long (argc, argv, optstring, longopts, NULL);
		if (c == -1)
			break;

		switch (c) {
		case '0':
			printf("option: %s\n", longopts[0].name);
			break;
		case 'a':
			params->all = 1;
			break;
		case 'l':
			params->long_list = 1;
			break;
		case 'R':
			params->recursive = 1;
			break;
		default:
			printf("bad parameters\n");
			#ifdef LS_DEBUG
			printf("getopt returned %c\n", c);
			#endif
			return -1;
		}
	}

	return 0;
}

void file_llist(char *dir, struct stat *stat)
{
	char s[11];
	char times[20];
	struct passwd *pwd;
	struct group *grp;
	char *tmp = NULL;

	memset(s, 0, 10);

	if (S_ISREG(stat->st_mode))
		s[0] = '-';
	if (S_ISBLK(stat->st_mode))
		s[0] = 'b';
	if (S_ISDIR(stat->st_mode))
		s[0] = 'd';
	if (S_ISCHR(stat->st_mode))
		s[0] = 'c';
	if (S_ISLNK(stat->st_mode))
		s[0] = 'l';
	if (S_ISSOCK(stat->st_mode))
		s[0] = 's';
	if (S_ISFIFO(stat->st_mode))
		s[0] = 'p';
	if (s[0] == 0)
		s[0] = '?';
	
	if (stat->st_mode & S_IRUSR)
		s[1] = 'r';
	else
		s[1] = '-';
	if (stat->st_mode & S_IWUSR)
		s[2] = 'w';
	else
		s[2] = '-';
	if (stat->st_mode & S_IXUSR)
		s[3] = 'x';
	else
		s[3] = '-';
	
	if (stat->st_mode & S_IRGRP)
		s[4] = 'r';
	else
		s[4] = '-';
	if (stat->st_mode & S_IWGRP)
		s[5] = 'w';
	else
		s[5] = '-';
	if (stat->st_mode & S_IXGRP)
		s[6] = 'x';
	else
		s[6] = '-';

	if (stat->st_mode & S_IROTH)
		s[7] = 'r';
	else
		s[7] = '-';
	if (stat->st_mode & S_IWOTH)
		s[8] = 'w';
	else
		s[8] = '-';
	if (stat->st_mode & S_IXOTH)
		s[9] = 'x';
	else
		s[9] = '-';

	s[10] = '\0';

	pwd = getpwuid(stat->st_uid);
	grp = getgrgid(stat->st_gid);

	printf("%s ", s);

	memset(times, 0, 20);
	snprintf(times, 20, "%d", max_links);
	#ifdef LS_DEBUG
	printf("%d, %s, %d\n", max_links, times, strlen(times));
	#endif
	printf("%1$*2$d ", stat->st_nlink, strlen(times));

	printf("%s %s ", pwd->pw_name, grp->gr_name);

	memset(times, 0, 20);
	snprintf(times, 20, "%d", max_filesize);
	#ifdef LS_DEBUG
	printf("%d, %s, %d\n", max_filesize, times, strlen(times));
	#endif
	printf("%1$*2$ld ", stat->st_size, strlen(times));

	memset(times, 0, 20);
	strftime(times, 20, "%b %d %R", localtime(&stat->st_mtime));

	tmp = strdup(dir);
	if (tmp) {
		printf("%s %s\n", times, basename(tmp));
		free(tmp);
	}
}

char out_buff[BUFSIZ][BUFSIZ];
static int obindex;

void init_ob()
{
	int i;
	obindex = 0;
	while (i < BUFSIZ) 
		memset(out_buff[i++], '\0', BUFSIZ);
}

void file_slist(char *bname)
{
	if (isatty(STDOUT_FILENO)) {
		sprintf(out_buff[obindex++], "%s", bname);
	} else {
		printf("%s\n", bname);
	}
}

void print_all()
{
	struct winsize ws;
	int ret;
	int longest_item = 0;
	int tt_length = 0;
	int min_rows, max_rows, max_cols;

	ret = ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
	if (ret != 0) {
		perror("ioctl error");
		return;
	}
	#ifdef LS_DEBUG
	printf("row %d, col %d, oni %d\n", ws.ws_row, ws.ws_col, obindex);
	#endif

	for(ret = 0; ret < obindex; ret++) {
		int item_leng = strlen(out_buff[ret]);
		#ifdef LS_DEBUG
		printf("out_buff[%d]:%s,", ret, out_buff[ret]);
		printf("len %d\n", strlen(out_buff[ret]));
		#endif

		tt_length += item_leng;
		tt_length += 2;
		if (item_leng > longest_item)
			longest_item = item_leng;
		#ifdef LS_DEBUG
		printf("tt %d, li %d\n", tt_length, longest_item);
		#endif
	}

	min_rows = tt_length / ws.ws_col + 1;
	max_rows = (longest_item + 2) * obindex / ws.ws_col + 1;
	max_cols = obindex / min_rows + 1;

	#ifdef LS_DEBUG
	printf("min/max rows:%d %d\n", min_rows, max_rows);
	#endif
	
	/* figure out the best rows number*/
	int best_row = 0;
	int col_len[max_cols];
	for (ret = min_rows; ret <= max_rows; ret++) {
		int smax_col_len = 0;
		int i, j;

		for (i = 0; i < max_cols; i++)
			col_len[i] = 0;
		int k = 0;
		for (i = 0 + ret; ; i = i + ret) {
			int max_col_len = 0;
			for (j = i - ret; j < i && j < obindex; j++) {
				int item_len = strlen(out_buff[j]);
				#ifdef LS_DEBUG
				//printf("item %d len %d\n", j, item_len);
				#endif
				if (max_col_len < item_len)
					max_col_len = item_len;
			}
			if (max_col_len == 0)
				break;

			smax_col_len += max_col_len;
			smax_col_len += 2;
			#ifdef LS_DEBUG
			printf("col %d,len %d,", k, max_col_len);
			printf("smax_col_len %d\n", smax_col_len);
			#endif
			col_len[k++] = max_col_len;
		}
		if (smax_col_len < ws.ws_col) {
			#ifdef LS_DEBUG
			printf("best rows: %d, smcl %d\n", ret, smax_col_len);
			#endif
			best_row = ret;
			break;
		}
		/* min/max cols cal may not be right*/
		if (ret == max_rows && best_row == 0)
			max_rows++;
	}

	/* print according to best_row*/
	int i, j, k = 0;
	for (i = 0; i < best_row; i++) {
		for (j = i, k = 0; j < obindex; j = j + best_row) {
			//printf("%1$*2$s  ", out_buff[j], col_len[i]);
			printf("%-*s  ", col_len[k++], out_buff[j]);
		}
		printf("\n");
	}
}

int list_dir(char *dir, struct ls_param *params)
{
	struct stat buf;
	int ret;
	struct dirent **namelist;
	int n = 0, i = 0, j = 0, dirno = 0;
	char *dirs[BUFSIZ];

	#ifdef LS_DEBUG
	printf("dir %s\n", dir);
	#endif

	if (params->recursive) {
		printf("%s:\n", dir);
	}

	n = scandir(dir, &namelist, 0, alphasort);
	if (n < 0) {
		perror("scandir");
		//free(namelist);
	} else {
		while (i < n) {
			char *item = (char *)malloc(BUFSIZ);
			sprintf(item, "%s/%s", dir,namelist[i]->d_name);

			ret = stat(item, &buf);
			if (ret != 0) {
				printf("stat error:%s %s\n", item, 
						strerror(errno));
				i++;
				continue;
			}
		
			if (S_ISDIR(buf.st_mode)) { 
				char *dir_copy = strdup(item);
				char *bname = basename(dir_copy);

				if (strcmp(bname, ".") != 0 &&
				    strcmp(bname, "..") != 0)
					dirno++;
			}

			if (buf.st_size > max_filesize)
				max_filesize = buf.st_size;
		
			if (buf.st_nlink > max_links)
				max_links = buf.st_nlink;
		
			free(namelist[i]);
			free(item);
			i++;
		}
		free(namelist);
	}

	for (i = 0; i < dirno; i++)
		while (dirs[i] == NULL)
			dirs[i] = (char *)malloc(BUFSIZ);

	i = 0;
	n = 0;
	n = scandir(dir, &namelist, 0, alphasort);
	if (n < 0) {
		perror("scandir");
		//free(namelist);
		return -1;
	} 
	while (i < n) {
		char *item = (char *)malloc(BUFSIZ);
		sprintf(item, "%s/%s", dir,namelist[i]->d_name);
		#ifdef LS_DEBUG
		printf("item %s\n", item);
		#endif

		ret = stat(item, &buf);
		if (ret != 0) {
			printf("stat error:%s %s\n", item, strerror(errno));
			i++;
			continue;
		}
	
		if (S_ISDIR(buf.st_mode)) {
			char *dir_copy = strdup(item);
			char *bname = basename(dir_copy);
			char *dir_copy1 = strdup(item);
			char *dname = dirname(dir_copy1);
	
			if (strcmp(bname, ".") == 0) {
				if (params->all) {
					if (params->long_list)
						file_llist(item, &buf);
					else
						file_slist(bname);
						//printf("%s  ", bname);
				}
				free(namelist[i]);
				free(item);
				i++;
				continue;
			}
	
			if (strcmp(bname, "..") == 0) {
				if (params->all) {
					if (params->long_list)
						file_llist(item, &buf);
					else
						file_slist(bname);
						//printf("%s  ", bname);
				}
				free(namelist[i]);
				free(item);
				i++;
				continue;
			}
	
			if (bname[0] == '.' && strlen(bname) > 1) {
				if (params->all) {
					/*
			 		* show before recurse for directory
			 		*/
					if (params->long_list)
						file_llist(item, &buf);
					else {
						file_slist(bname);
						//printf("%s  ", bname);
					}
					if (params->recursive)
						strcpy(dirs[j++], item);
						//list_dir(item, params);
				}
				free(namelist[i]);
				free(item);
				i++;
				continue;
			}
	
			/*
			 * show before recurse for directory
			 */
			if (params->long_list)
				file_llist(item, &buf);
			else {
				file_slist(bname);
				//printf("%s  ", bname);
			}
			if (params->recursive)
				strcpy(dirs[j++], item);
				//list_dir(item, params);
	
			free(dir_copy);
			free(dir_copy1);
		}
		else {
			char *dir_copy = strdup(item);
			char *bname = basename(dir_copy);
			if (bname[0] == '.' && strlen(bname) > 1) {
				if (params->all) {
					/*
			 		* show . start files
			 		*/
					if (params->long_list)
						file_llist(item, &buf);
					else
						file_slist(bname);
				}
				free(namelist[i]);
				free(item);
				i++;
				continue;
			}
			if (params->long_list)
				file_llist(item, &buf);
			else {
				char *s = strdup(item);
				file_slist(basename(s));
				//printf("%s  ", basename(s));
				free(s);
			}
		}
	
		free(namelist[i]);
		free(item);
		i++;
	}
	free(namelist);

	//if (params->long_list == 0)
	//printf("\n");

	for (i = 0; i < j; i++) {
		#ifdef LS_DEBUG
		printf("dirs[%d]: %s\n", i, dirs[i]);
		#endif
		print_all();
		printf("\n");
		init_ob();
		list_dir(dirs[i], params);
	}

	//for (i = 0; i < j; i++)
	//	free(dirs[i]);

	return 0;
}

int main(int argc, char **argv)
{
	int list;
	int ret = 0;
	struct ls_param params;
	char *target;

	setlocale(LC_ALL, "");

	init_param(&params);
	parse_params(argc, argv, &params);

	if (argv[optind] == NULL) {
		ret = list_dir(".", &params);
		if (isatty(STDOUT_FILENO))
			print_all();
	}

	int dflag = 0;
	while (argv[optind]) {
		#ifdef LS_DEBUG
		printf("argc %d, argv[%d]: %s\n", argc, optind, argv[optind]);
		#endif
		init_ob();
		if (argv[optind + 1])
			dflag = 1;

		if (params.recursive || dflag)
			printf("%s:\n", argv[optind]);

		ret = list_dir(argv[optind], &params);

		if (isatty(STDOUT_FILENO))
			print_all();
		if (argv[optind + 1] != NULL)
			printf("\n");
		optind++;
	}

	return ret;
}
