#include <stdio.h>
#include <wdk/cdb.h>
#include <libutils/shutils.h>

int main(int argc, char *argv[])
{
	int reg, val;

	if(argc == 2)
	{
		sscanf(argv[1], "%x", &reg);
		switch(reg){
			case 0x11:
				reg = cdb_get_int("$bb_reg", 0);
				exec_cmd2("wd bb r 1 %x", reg);
				break;
			case 0x13:
				reg = cdb_get_int("$bb_reg", 0);
				exec_cmd2("wd bb r 2 %x", reg);
				break;
			case 0x15:
				reg = cdb_get_int("$bb_reg", 0);
				exec_cmd2("wd bb r 3 %x", reg);
				break;
			default:
				exec_cmd2("wd bb r 0 %x", reg);
				break;
		}
	}
	else if(argc == 3)
	{
		sscanf(argv[1], "%x", &reg);
		sscanf(argv[2], "%x", &val);
		switch(reg){
			case 0x10:
				cdb_set_int("$bb_reg", val);
				printf("SET BB%x = %02x\n", reg, val);
				break;
			case 0x11:
				reg = cdb_get_int("$bb_reg", 0);
				exec_cmd2("wd bb w 1 %x %x", reg, val);
				break;
			case 0x12:
				cdb_set_int("$bb_reg", val);
				printf("SET BB%x = %02x\n", reg, val);
				break;
			case 0x13:
				reg = cdb_get_int("$bb_reg", 0);
				exec_cmd2("wd bb w 2 %x %x", reg, val);
				break;
			case 0x14:
				cdb_set_int("$bb_reg", val);
				printf("SET BB%x = %02x\n", reg, val);
				break;
			case 0x15:
				reg = cdb_get_int("$bb_reg", 0);
				exec_cmd2("wd bb w 2 %x %x", reg, val);
				break;
			default:
				exec_cmd2("wd bb w 0 %x %x", reg, val);
				break;
		}
	}
	return 0;
}
