/* gtkcolorcombo - color_combo widget for gtk+
 * Copyright 1999-2001 Adrian E. Feiguin <feiguin@ifir.edu.ar>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


/**
 * SECTION: gtkcolorcombo
 * @short_description: A color combo widget for GTK.
 *
 * A GtkColorCombo is a GtkCOmbo which contains all kinds of colors.
 */

/**
 * GtkColorCombo:
 *
 * The GtkColorCombo struct contains only private data.
 * It should only be accessed through the functions described below.
 */



#include <string.h>
#include <stdio.h>
#include <math.h>
#include <gtk/gtkarrow.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtktable.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkcolorsel.h>
#include <gtk/gtkcolorseldialog.h>
#include <gdk/gdkkeysyms.h>
#include "gtkcombobutton.h"
#include "gtkcolorcombo.h"
#include "gtkextra-marshal.h"

static GtkWidget *dialog;

/* SIGNALS */
enum {
   CHANGED,
   LAST_SIGNAL
};

static gint color_combo_signals[LAST_SIGNAL] = {0};


static char *default_colors[]={
"black",
"brown",
"olive drab",
"dark green",
"dark sea green",
"dark blue",
"dark cyan",
"grey80",
"dark red",
"coral",
"yellow green",
"sea green",
"aquamarine3",
"blue",
"steel blue",
"grey50",
"red",
"orange",
"lime green",
"green",
"aquamarine1",
"light blue",
"violet",
"grey40",
"pale violet red",
"gold1",
"yellow1",
"lawn green",
"turquoise1",
"SkyBlue1",
"purple2",
"grey25",
"light pink",
"light goldenrod",
"light yellow",
"light green",
"pale turquoise",
"light steel blue",
"lavender",
"white"
};

static char *xpm_color[]={
"18 18 3 1",
"      c None",
".     c #000000000000",
"X     c #111111111111",
"                  ",
" ................ ",
" .XXXXXXXXXXXXXX. ",
" .XXXXXXXXXXXXXX. ",
" .XXXXXXXXXXXXXX. ",
" .XXXXXXXXXXXXXX. ",
" .XXXXXXXXXXXXXX. ",
" .XXXXXXXXXXXXXX. ",
" .XXXXXXXXXXXXXX. ",
" .XXXXXXXXXXXXXX. ",
" .XXXXXXXXXXXXXX. ",
" .XXXXXXXXXXXXXX. ",
" .XXXXXXXXXXXXXX. ",
" .XXXXXXXXXXXXXX. ",
" .XXXXXXXXXXXXXX. ",
" .XXXXXXXXXXXXXX. ",
" ................ ",
"                  "};

static void         gtk_color_combo_class_init      (GtkColorComboClass *klass);
static void         gtk_color_combo_init            (GtkColorCombo      *color_combo);
static void         gtk_color_combo_destroy         (GtkObject     *color_combo);
static void         gtk_color_combo_realize         (GtkWidget *widget);
static void 	    gtk_color_combo_get_color_name  (GdkColor *color, 
						     gchar *name);
static void 	    color_to_hex		    (gint color, gchar string[5]);

static GtkComboButtonClass *parent_class = NULL;

static void
gtk_color_combo_class_init (GtkColorComboClass * klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  parent_class = g_type_class_ref (gtk_hbox_get_type ());
  object_class = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  object_class->destroy = gtk_color_combo_destroy;

  widget_class->realize = gtk_color_combo_realize;
 
  color_combo_signals[CHANGED]=g_signal_new("changed",
                                 G_TYPE_FROM_CLASS(object_class),
                                 G_SIGNAL_RUN_FIRST,
                                 G_STRUCT_OFFSET(GtkColorComboClass, changed),
				 NULL, NULL,
                                 gtkextra_VOID__INT_BOXED,
                                 G_TYPE_NONE,  
                                 2, G_TYPE_INT, GDK_TYPE_COLOR);

  klass->changed = NULL;
}

static void
gtk_color_combo_destroy (GtkObject * color_combo)
{
  gint i,j;

  GtkColorCombo *combo;
  combo=GTK_COLOR_COMBO(color_combo);

  if(combo && combo->button) /* patched by Mario Motta <mmotta@guest.net> */
   for(i=0; i<combo->nrows; i++)
    for(j=0; j<combo->ncols; j++)
      if(combo->button[i*combo->ncols+j]){
        gtk_widget_destroy(combo->button[i*combo->ncols+j]);
        combo->button[i*combo->ncols+j] = NULL;
      }
  
  if(combo->button){
    g_free(combo->button);
    combo->button = NULL;
  }

  if(combo->colors){
    g_free(combo->colors);
    combo->colors = NULL;
  }
 
  if(GTK_COLOR_COMBO(color_combo)->table){
    gtk_widget_destroy (GTK_COLOR_COMBO(color_combo)->table);
    GTK_COLOR_COMBO(color_combo)->table = NULL;
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (color_combo);
}



static void
gtk_color_combo_update (GtkWidget * widget, GtkColorCombo * color_combo)
{
  gint i,j;
  gint focus_row = -1, focus_col = -1;
  gint new_row = -1, new_col = -1;
  gint new_selection=FALSE;
  gint row,column;

  row=color_combo->row;
  column=color_combo->column;

  for(i=0 ; i<color_combo->nrows; i++)
    for(j=0; j<color_combo->ncols; j++){
      gint index = i*color_combo->ncols+j;
      if(gtk_widget_has_focus(color_combo->button[index])){
            focus_row=i;
            focus_col=j;
      }
      if(gtk_widget_get_state(color_combo->button[index])==GTK_STATE_ACTIVE){
        if(i != row || j != column){
            new_selection=TRUE;
            new_row=i;
            new_col=j;
        }
      }
    }

  if(!new_selection && focus_row >= 0 && focus_col >= 0){
     if(focus_row != row && focus_col != column){
       new_selection = TRUE;
       new_row=focus_row;
       new_col=focus_col;
     }
  }



  if(new_selection){
      if(row >= 0 && column >= 0){
          gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(color_combo->button[row*color_combo->ncols+column]), FALSE);
          gtk_widget_queue_draw(color_combo->button[row*color_combo->ncols+column]);
      }
      color_combo->row=new_row;
      color_combo->column=new_col;
      color_combo->selection = color_combo->colors[new_row*color_combo->ncols+new_col];

      g_signal_emit (GTK_OBJECT(color_combo), color_combo_signals[CHANGED], 0,
                  new_row * color_combo->ncols + new_col,
                  &color_combo->selection);

  }

  if(!new_selection && row >= 0 && column >= 0){
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(color_combo->button[row*color_combo->ncols+column]), TRUE);
      gtk_widget_queue_draw(color_combo->button[row*color_combo->ncols+column]);
      g_signal_emit (GTK_OBJECT(color_combo),
                       color_combo_signals[CHANGED], 0,
                       row * color_combo->ncols + column,
                       &color_combo->colors[row*color_combo->ncols+column]);

  }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_COMBO_BUTTON(color_combo)->arrow), FALSE);

  gtk_grab_remove(GTK_COMBO_BUTTON(color_combo)->popwin);
  gdk_pointer_ungrab(GDK_CURRENT_TIME);
  gtk_widget_hide(GTK_COMBO_BUTTON(color_combo)->popwin);
  return;
}

static void
gtk_color_combo_init (GtkColorCombo * color_combo)
{
  GtkWidget *widget;

  widget=GTK_WIDGET(color_combo);

  color_combo->row = -1;
  color_combo->column = -1;

  gdk_color_black(gtk_widget_get_colormap(widget), &color_combo->selection);
}

static gboolean
pick_color(GtkWidget *widget, gpointer data)
{
  GtkColorCombo *combo;
  GdkColor color;
  
  combo = GTK_COLOR_COMBO(data);

  gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(dialog)->colorsel), &color);

  gdk_color_alloc(gdk_colormap_get_system(), &color);


  combo->selection = color;
  g_signal_emit (GTK_OBJECT(combo), color_combo_signals[CHANGED], 0,
                   -1, &color);
  return FALSE;
}

static gboolean
gtk_color_combo_customize(GtkButton *button, gpointer data)
{
  GtkColorCombo *combo = GTK_COLOR_COMBO(data);
  GdkColor color;

  if(combo->row == -1 || combo->column == -1) return FALSE;

  dialog = gtk_color_selection_dialog_new("Pick a color");
  color = gtk_color_combo_get_color_at(combo, combo->row, combo->column);

  gtk_color_selection_set_has_palette (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel), TRUE);
  gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(dialog)->colorsel), &color);
  gtk_window_set_modal (GTK_WINDOW(dialog),TRUE);

  gtk_widget_show(dialog);

  g_signal_connect (GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(dialog)->ok_button),
                      "clicked", (void *)pick_color, combo);

  g_signal_connect_swapped (
	GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(dialog)->ok_button),
        "clicked", (void *)gtk_widget_destroy, 
         GTK_OBJECT (dialog));
  g_signal_connect_swapped (
	GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(dialog)->cancel_button),
        "clicked", (void *)gtk_widget_destroy, GTK_OBJECT (dialog));

 return FALSE;
}

static void
gtk_color_combo_realize(GtkWidget *widget)
{
  GtkComboButton *combo;
  GtkColorCombo *color_combo;
  GdkPixmap *color_pixmap;
  GtkWidget *pixmap;
  GtkWidget *box;
  gchar color_string[21];
  gint i,j,n;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_COLOR_COMBO (widget));

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  combo = GTK_COMBO_BUTTON(widget);
  color_combo = GTK_COLOR_COMBO(widget);

  box = gtk_vbox_new(FALSE,0);
  color_combo->table = gtk_table_new (color_combo->nrows, color_combo->ncols, TRUE);

  color_combo->button = g_new0(GtkWidget *, color_combo->nrows*color_combo->ncols);

  for(i = 0; i < color_combo->nrows; i++){
    for(j = 0; j < color_combo->ncols; j++){
        gint index = i*color_combo->ncols+j;
        color_combo->button[index] = gtk_toggle_button_new();
        gtk_button_set_relief(GTK_BUTTON(color_combo->button[index]),
                              GTK_RELIEF_NONE);
        gtk_table_attach (GTK_TABLE(color_combo->table),
                          color_combo->button[index],
                          j, j+1, i, i+1, GTK_SHRINK, GTK_SHRINK, 0, 0);

        gtk_widget_set_size_request(color_combo->button[index], 24, 24);
        gtk_widget_show(color_combo->button[index]);
        g_signal_connect (GTK_OBJECT (color_combo->button[index]), "toggled",
                            (void *) gtk_color_combo_update,
                            color_combo);

    }
  }


  gtk_container_add(GTK_CONTAINER(GTK_COMBO_BUTTON(color_combo)->frame), box);
  gtk_box_pack_start(GTK_BOX(box), color_combo->table, TRUE, TRUE, 0);
  gtk_widget_show(box);
  gtk_widget_show(color_combo->table);

  color_combo->custom_button = gtk_button_new_with_label ("Pick a new color");
  gtk_table_attach (GTK_TABLE(color_combo->table),
                    color_combo->custom_button,
                    0, color_combo->ncols,
                    color_combo->nrows, color_combo->nrows+1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(color_combo->custom_button); 

  g_signal_connect (GTK_OBJECT(color_combo->custom_button),"clicked",
                      (void *) gtk_color_combo_customize, color_combo);

  n=0;

  for(i=0; i<color_combo->nrows; i++)
   for(j=0; j<color_combo->ncols; j++){
       gchar *prev_string = xpm_color[3];
       gchar name[21];

       gtk_color_combo_get_color_name(&color_combo->colors[n], name);
       sprintf(color_string,"X     c %s",name);

       xpm_color[3]=color_string;

       color_pixmap=gdk_pixmap_create_from_xpm_d(
                          gtk_widget_get_window(widget),
                          NULL,
                          &(gtk_widget_get_style(widget)->bg[GTK_STATE_NORMAL]),
                          xpm_color);
       pixmap=gtk_image_new_from_pixmap(color_pixmap, NULL);
       gtk_container_add(GTK_CONTAINER(color_combo->button[n]), pixmap);
       gtk_widget_show(pixmap);
       gdk_pixmap_unref(color_pixmap);
       xpm_color[3] = prev_string;
       n++;
    }

  g_signal_connect (GTK_OBJECT (combo->button), "clicked",
	              (void *) gtk_color_combo_update, 
                      color_combo);

  gtk_color_combo_update(NULL, color_combo);

}

static void
gtk_color_combo_get_color_name(GdkColor *color, gchar *name)
{
  gchar red[5];
  gchar green[5];
  gchar blue[5];

  color_to_hex(color->red, red);
  color_to_hex(color->green, green);
  color_to_hex(color->blue, blue);

  g_snprintf(name,21,"#%s%s%s",red,green,blue);
}

static void
color_to_hex(gint color, gchar string[5])
{
  gint i,n;

  for(i=3; i>=0; i--){
     n=color/pow(16,i);
     color-=n*pow(16,i);
     if(n < 10)
       string[3-i]='0'+n;
     else
       string[3-i]='A'+n-10;
  }
  string[4]='\0';
}     

GType
gtk_color_combo_get_type ()
{
  static GType color_combo_type = 0;

  if (!color_combo_type)
    {
      color_combo_type = g_type_register_static_simple (
		gtk_combo_button_get_type (),
		"GtkColorCombo",
		sizeof (GtkColorComboClass),
        	(GClassInitFunc) gtk_color_combo_class_init,
		sizeof (GtkColorCombo),
		(GInstanceInitFunc) gtk_color_combo_init,
		0 );
    }
  return color_combo_type;
}

GtkWidget *
gtk_color_combo_new ()
{
  GtkWidget * color_combo;

  color_combo = gtk_widget_new (gtk_color_combo_get_type (), NULL);

  gtk_color_combo_construct((GtkColorCombo *)color_combo);

  return(color_combo);
}

/**
 * gtk_color_combo_construct:
 * @color_combo: the #GtkColorCombo widget.
 * 
 * Initialize a #GtkColorCombo object.
 */
void
gtk_color_combo_construct(GtkColorCombo *color_combo)
{
  GdkColor color;
  gint i,j,n;

  color_combo->nrows = 5;
  color_combo->ncols = 8;
  n = color_combo->nrows * color_combo->ncols;
  color_combo->colors = g_new0(GdkColor, n);

  n=0;
 
  for(i=0; i<color_combo->nrows; i++)
   for(j=0; j<color_combo->ncols; j++){

       gdk_color_parse(default_colors[n], &color);
       gdk_color_alloc(gtk_widget_get_colormap(GTK_WIDGET(color_combo)), &color);
       n++;

       color_combo->colors[n-1]=color;
   }


}

/**
 * gtk_color_combo_new_with_values:
 * @nrows: number of rows in #GtkColorCombo.
 * @ncols: number of columns in #GtkColorCombo.
 * @colors: #GdkColor color.
 * 
 * Creates a new #GtkColorCombo widget having @nrows rows and @ncols columns with #GdkColor color.
 *  
 * Returns: the newly-created #GtkColorCombo widget.
 */
GtkWidget *
gtk_color_combo_new_with_values (gint nrows, gint ncols, GdkColor *colors)
{
  GtkWidget * color_combo;

  color_combo = gtk_widget_new(gtk_color_combo_get_type(), NULL);

  gtk_color_combo_construct_with_values((GtkColorCombo *)color_combo, nrows, ncols, colors);
  return(color_combo);
}

/**
 * gtk_color_combo_construct_with_values:
 * @color_combo: the #GtkColorCombo widget.
 * @nrows: number of rows in #GtkColorCombo.
 * @ncols: number of columns in #GtkColorCombo.
 * @colors: #GdkColor color.
 * 
 * Initialize a #GtkColorCombo object with specified values.
 */
void
gtk_color_combo_construct_with_values(GtkColorCombo *color_combo,
				      gint nrows, gint ncols, 
                                      GdkColor *colors)
{
  gint i,j,n;

  color_combo->nrows = nrows;
  color_combo->ncols = ncols;
  n = color_combo->nrows * color_combo->ncols;
  color_combo->colors = g_new0(GdkColor, n);

  n=0;
  
  for(i=0; i<color_combo->nrows; i++)
    for(j=0; j<color_combo->ncols; j++)
       color_combo->colors[n-1] = *colors; // RRR color to colors

}

/**
 * gtk_color_combo_get_color_at:
 * @color_combo: a #GtkColorCombo widget.
 * @row: row number  in #GtkColorCombo.
 * @col: column number in #GtkColorCombo.
 * 
 * Get the color in the combo at the specified row,column.
 *  
 * Returns: a #GdkColor at @row,@col in @color_combo
 */
GdkColor 
gtk_color_combo_get_color_at(GtkColorCombo *color_combo, gint row, gint col)
{
   return color_combo->colors[row*color_combo->ncols+col];
}

/* Returns best match for a given color */

/**
 * gtk_color_combo_find_color:
 * @color_combo: a #GtkColorCombo widget.
 * @color: a #GdkColor object.
 * @row: row number in #GtkColorCombo.
 * @col: column number in #GtkColorCombo.
 * 
 * Find the best match for a given color.
 */
void
gtk_color_combo_find_color(GtkColorCombo *color_combo,
                           GdkColor *color, gint *row, gint *col)
{
   GdkColor combo_color;
   gint i, j;
   gdouble dist = 114000.0;
   gdouble d, dr, dg, db;

   *row = -1;
   *col = -1;

   for(i = 0; i < color_combo->nrows; i++){ 
     for(j = 0; j < color_combo->ncols; j++){ 
        combo_color = gtk_color_combo_get_color_at(color_combo, i, j);

        if(gdk_color_equal(color, &combo_color)){
                   *row = i;
                   *col = j;
                   return;
        }
       
        dr = fabs(color->red - combo_color.red); 
        dg = fabs(color->green - combo_color.green);  
        db = fabs(color->blue - combo_color.blue); 

        d = dr + dg + db;
/*
        printf("%d %d %d // %d %d %d\n",color->red,color->green,color->blue,combo_color.red,combo_color.green,combo_color.blue);
        printf("%f\n",d);
*/

        if(d < dist){
            dist = d;
            *row = i; 
            *col = j; 
        }
     }
   }
} 

/**
 * gtk_color_combo_get_selection:
 * @combo: a #GtkColorCombo widget.
 * 
 * Get the color in the combo at selection .
 *  
 * Returns: a #GdkColor of the selection. 
 */
GdkColor
gtk_color_combo_get_selection(GtkColorCombo *combo)
{
  return combo->selection;
}

