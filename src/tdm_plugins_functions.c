#include "g_local.h"
#include "tdm_plugins_internal.h"

long quake2InfoSetValueForKey( unsigned int wParam, long lParam )
{
	struct sInfoStrings *info = (struct sInfoStrings *)lParam;

	if ( !info || !info->infoString || !info->key || !info->value )
		return 0;

	Info_SetValueForKey( info->infoString, info->key, info->value );

	return 0;
}

long quake2InfoValueForKey( unsigned int wParam, long lParam )
{
	struct sInfoStrings *info = (struct sInfoStrings *)lParam;
	char *tmp;
	size_t value_size = (size_t)wParam;

	if ( !info || !info->infoString || !info->key || !info->value || value_size <= 0 )
		return 0;

	tmp = Info_ValueForKey( info->infoString, info->key );

	strncpy( info->value, tmp, value_size-1 );

	return 0;
}
