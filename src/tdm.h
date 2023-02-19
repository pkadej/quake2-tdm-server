#define	AD_SERVER	0x00000001
#define	AD_TEAMS	0x00000002
#define	AD_BAN		0x00000004
#define	AD_MATCH	0x00000008
#define	AD_ALL		0x00000010

struct admin_s
{
	struct admin_s *prev;
	struct admin_s *next;
	char nick[17];
	char password[16];
	int	time;
	int	flags;
	qboolean used;
	qboolean judge;
};

typedef struct ban_s ban_t;

struct ban_s
{
	ban_t *prev;
	ban_t *next;
	char ip[20];
	char nick[17];
	char giver[17];
	time_t whenBaned;
	time_t whenRemove;
};