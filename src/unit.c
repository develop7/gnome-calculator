#include <string.h>

#include "unit.h"
#include "mp-serializer.h"
#include "mp-equation.h"
#include "currency-manager.h" // FIXME: Move out of here

struct UnitPrivate
{
    gchar *name;
    gchar *display_name;
    gchar *format;
    GList *symbols;
    gchar *from_function;
    gchar *to_function;
    MpSerializer *serializer;
};

G_DEFINE_TYPE (Unit, unit, G_TYPE_OBJECT);


Unit *
unit_new(const gchar *name,
         const gchar *display_name,
         const gchar *format,
         const gchar *from_function,
         const gchar *to_function,
         const gchar *symbols)
{
    Unit *unit = g_object_new(unit_get_type(), NULL);
    gchar **symbol_names;
    int i;

    unit->priv->name = g_strdup(name);
    unit->priv->display_name = g_strdup(display_name);
    unit->priv->format = g_strdup(format);
    unit->priv->from_function = g_strdup(from_function);
    unit->priv->to_function = g_strdup(to_function);
    symbol_names = g_strsplit(symbols, ",", 0);
    for (i = 0; symbol_names[i]; i++)
        unit->priv->symbols = g_list_append(unit->priv->symbols, g_strdup(symbol_names[i]));
    g_free(symbol_names);

    return unit;
}


const gchar *
unit_get_name(Unit *unit)
{
    return unit->priv->name;
}


const gchar *
unit_get_display_name(Unit *unit)
{
    return unit->priv->display_name;
}


gboolean
unit_matches_symbol(Unit *unit, const gchar *symbol)
{
    GList *iter;

    for (iter = unit->priv->symbols; iter; iter = iter->next) {
        gchar *s = iter->data;
        if (strcmp(s, symbol) == 0)
            return TRUE;
    }

    return FALSE;
}


const GList *
unit_get_symbols(Unit *unit)
{
    return unit->priv->symbols;
}


static int
variable_is_defined(const char *name, void *data)
{
    return TRUE;
}


static int
get_variable(const char *name, MPNumber *z, void *data)
{
    MPNumber *x = data;
    mp_set_from_mp(x, z);
    return TRUE;
}


static void
solve_function(const gchar *function, const MPNumber *x, MPNumber *z)
{
    MPEquationOptions options;
    int ret;

    memset(&options, 0, sizeof(options));
    options.base = 10;
    options.wordlen = 32;
    options.variable_is_defined = variable_is_defined;
    options.get_variable = get_variable;
    options.callback_data = (void *)x;
    ret = mp_equation_parse(function, &options, z, NULL);
    if (ret)
        g_warning("Failed to convert value: %s", function);
}


void
unit_convert_from(Unit *unit, const MPNumber *x, MPNumber *z)
{
    if (unit->priv->from_function)
        solve_function(unit->priv->from_function, x, z);
    else {
        // FIXME: Hack to make currency work
        const MPNumber *r;
        r = currency_manager_get_value(currency_manager_get_default(), unit->priv->name);
        mp_divide(x, r, z);
    }
}


void
unit_convert_to(Unit *unit, const MPNumber *x, MPNumber *z)
{
    if (unit->priv->from_function)
        solve_function(unit->priv->to_function, x, z);
    else {
        // FIXME: Hack to make currency work
        const MPNumber *r;
        r = currency_manager_get_value(currency_manager_get_default(), unit->priv->name);
        mp_multiply(x, r, z);
    }
}


gchar *
unit_format(Unit *unit, MPNumber *x)
{
    gchar *number_text, *text;

    number_text = mp_serializer_to_string(unit->priv->serializer, x);
    text = g_strdup_printf(unit->priv->format, number_text);
    g_free(number_text);

    return text;
}


static void
unit_class_init(UnitClass *klass)
{
    g_type_class_add_private(klass, sizeof(UnitPrivate));
}


static void
unit_init(Unit *unit)
{
    unit->priv = G_TYPE_INSTANCE_GET_PRIVATE(unit, unit_get_type(), UnitPrivate);
    unit->priv->serializer = mp_serializer_new(MP_DISPLAY_FORMAT_AUTOMATIC, 10, 2);
}