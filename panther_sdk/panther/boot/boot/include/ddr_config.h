

// �����������꣬����ѡ��DDR3����DDR2��ͬʱֻ������һ������Ч
//#define CONFIG_USE_DDR3  // ��ѡ��DDR3ʱ, enable�����
//#define CONFIG_USE_DDR2  // ��ѡ��DDR2ʱ, enable�����

// ��������꣬��������ѡ����FPGA����Silicon IC
#define CONFIG_BOARD_WITH_CHIP  // Silicon IC

// ������Щ�꣬�������õ�ǰDDR�Ĺ���Ƶ��
#ifdef CONFIG_USE_DDR2
// �����������꣬�����������õ�ǰDDR�Ĺ���Ƶ�ʣ�ͬʱֻ������һ������Ч
#define CONFIG_FREQ396  // �����������꣬DDR2��ʱ��Ƶ����396MHz
#undef  CONFIG_FREQ396  // ���δ��������꣬DDR2��ʱ��Ƶ����531MHz
#endif
#ifdef CONFIG_USE_DDR3
// �����������꣬�����������õ�ǰDDR�Ĺ���Ƶ�ʣ�ͬʱֻ������һ������Ч
//#define CONFIG_FREQ666  // �����������꣬DDR3��ʱ��Ƶ����666MHz
#undef  CONFIG_FREQ666  // ���δ��������꣬DDR3��ʱ��Ƶ����528MHz
#endif

#define DDR_SIZE_INFO_ADDR 0xBF005510UL     // unit: MB
