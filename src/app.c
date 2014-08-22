#include <pebble.h>
#include <pebble_fonts.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define PAUSED 2
#define WORKING 1
#define RESTING 0 
#define ENDED -1 
  
  
int series_num = 1;
int series_time =  60;
int rest_time = 0;

static Window *window_series_num;  
static Window *window_series_time;  
static Window *window_rest_time;  
static Window *window_work_pre;  
static Window *window_work_progress;  
static Window *window_work_end;  

static Window *windows[6];

static TextLayer *layer_series_num_title;
static TextLayer *layer_series_num;
static TextLayer *layer_series_time_title;
static TextLayer *layer_series_time;
static TextLayer *layer_rest_time_title;
static TextLayer *layer_rest_time;
static TextLayer *layer_work_pre;
static TextLayer *layer_work_title;
static TextLayer *layer_work;
static TextLayer *layer_work_end;
static ScrollLayer *layer_work_end_scroll;

static char label_series_num[] = "#   ";
static char label_series_time[6];
static char label_rest_time[6];
static char label_work_pre[255];
static char label_work_title[255];
static char label_work[6];
static char label_work_end[512];

static int status = WORKING;
static int mode = 0;
static int serie_actual = 0;
static int rest_actual = 0;
static int *series_times;
static int *rest_times;

static AppTimer *timer;
static const int vert_scroll_text_padding = 4;

static void draw_window(Window *window){
  // Ventana de seleccion de numero de series
  if(window == windows[0]){
    snprintf(label_series_num,16,"# %d",series_num);
    text_layer_set_text(layer_series_num,label_series_num);
  }
  
  if(window == windows[1]){
     if(series_time > 0){
       snprintf(label_series_time,6,"%02d:%02d",(int)series_time / 60,series_time % 60);
       text_layer_set_text(layer_series_time,label_series_time);
     } else{
       text_layer_set_text(layer_series_time,"CLICK!");
     }
   }
  
  if(window == windows[2]){
     if(rest_time >= 0){
       snprintf(label_rest_time,6,"%02d:%02d",(int)rest_time / 60,rest_time % 60);
       text_layer_set_text(layer_rest_time,label_rest_time);
     } else{
       text_layer_set_text(layer_rest_time,"CLICK!");
     }
   }
  
  if(window == windows[3]){
    int buff_length = 0;
    buff_length = snprintf(label_work_pre,255,"SERIES: %d\n", series_num);
    if(series_time >0){
      buff_length += snprintf(label_work_pre + buff_length,255 - buff_length,"SERIES TIME: %02d:%02d\n",(int)series_time / 60,series_time % 60);
    }else{
      buff_length += snprintf(label_work_pre + buff_length,255 - buff_length,"UNTIL CLICK\n");
    }
    if(rest_time > 0){
      buff_length += snprintf(label_work_pre + buff_length,255 - buff_length,"REST TIME: %02d:%02d\n",(int)rest_time / 60,rest_time % 60);
    }
    if(rest_time == 0){
      buff_length += snprintf(label_work_pre + buff_length,255 - buff_length,"NO REST!!!\n");
    }
    if(rest_time < 0){
      buff_length += snprintf(label_work_pre + buff_length,255 - buff_length,"REST UNTIL CLICK\n");
    }
    snprintf(label_work_pre + buff_length,255 - buff_length,"\nCLICK TO START");
    text_layer_set_text(layer_work_pre,label_work_pre);
  }
  
  if(window == windows[4]){
    int segundos = 0, minutos = 0;
    switch(status){
      case RESTING:
        snprintf(label_work_title,255,"RESTING\nSERIES: %d", rest_actual+1);
        if(rest_time < 0 || mode == 0){
          minutos = (int)rest_times[rest_actual] / 60;
          segundos = rest_times[rest_actual] % 60;
        } else{
          minutos = (int)(rest_time - rest_times[rest_actual]) / 60;
          segundos = (rest_time - rest_times[rest_actual]) % 60;
        }
     break;
     case WORKING:
        snprintf(label_work_title,255,"WORKING OUT\nSERIES: %d", serie_actual+1);
        if(series_time == 0 || mode == 0){
          minutos = (int)series_times[serie_actual] / 60;
          segundos = series_times[serie_actual] % 60;
        } else{
          minutos = (int)(series_time - series_times[serie_actual]) / 60;
          segundos = (series_time - series_times[serie_actual]) % 60;
        }
      break;
     }
    snprintf(label_work,6,"%02d:%02d", minutos,segundos);
    text_layer_set_text(layer_work_title,label_work_title);
    text_layer_set_text(layer_work,label_work);
  }

  if(window == windows[5]){
  int buffer_length = snprintf(label_work_end,512,"SUMMARY:\n\n");
    int total = 0;
    int i;
    for(i = 0; i < series_num; i++){
      buffer_length += snprintf(label_work_end+buffer_length,512-buffer_length,"SERIES %d: %02d:%02d\n", i, (int)series_times[i]/60,series_times[i] % 60);
      total += series_times[i];
      if(rest_time != 0){
        buffer_length += snprintf(label_work_end+buffer_length,512-buffer_length,"REST    %d: %02d:%02d\n", i, (int)rest_times[i]/60,rest_times[i] % 60);
        total += rest_times[i];
      }
    }
    snprintf(label_work_end+buffer_length,512-buffer_length,"\nTOTAL: %02d:%02d\n", (int)total/60,total % 60);
    text_layer_set_text(layer_work_end, label_work_end);  
    
    //text_layer_set_text(layer_work_end,"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam quam tellus, fermentu  m quis vulputate quis, vestibulum interdum sapien. Vestibulum lobortis pellentesque pretium. Quisque ultricies purus e  u orci convallis lacinia. Cras a urna mi. Donec convallis ante id dui dapibus nec ullamcorper erat egestas. Aenean a m  auris a sapien commodo lacinia. Sed posuere mi vel risus congue ornare. Curabitur leo nisi, euismod ut pellentesque se  d, suscipit sit amet lorem. Aliquam eget sem vitae sem aliquam ornare. In sem sapien, imperdiet eget pharetra a, lacin  ia ac justo. Suspendisse at ante nec felis facilisis eleifend.");
    GRect bounds = layer_get_bounds(window_get_root_layer(window));
    GSize max_size = text_layer_get_content_size(layer_work_end);
    // Pongo el maximo de ancho de la pantalla hasta ver porque esta podienndo 109 en vez de 144:
    max_size.w = 144;
    text_layer_set_size(layer_work_end, max_size);
    scroll_layer_set_content_size(layer_work_end_scroll, GSize(bounds.size.w, max_size.h + vert_scroll_text_padding));
  }
}

static void procesa_fin_serie(){
  vibes_long_pulse();
  serie_actual++;
  // Si era la ultima serie, cargamos la pagina de finalizacion
  if(serie_actual == series_num){
    status = ENDED;
    window_stack_push(windows[5], true);
  } else {
    if(rest_time > 0 || rest_time < 0) {
      status = RESTING;
    }
  }
}
static void procesa_fin_rest(){
  vibes_short_pulse();
  rest_actual++;
  status = WORKING;
}

static void timer_callback(void *context) {
  if(status != ENDED){
    if(status == WORKING){
      series_times[serie_actual]++;
      if(series_times[serie_actual]==series_time){
        procesa_fin_serie();
      } 
    }else{
      rest_times[rest_actual]++;
      if(rest_times[rest_actual]==rest_time){
        procesa_fin_rest();
      } 
    }
    timer = app_timer_register(1000, timer_callback, NULL);
    draw_window(windows[4]);
  }
}



static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(context == windows[0]){
      window_stack_push(windows[1], true);
  }
  if(context == windows[1]){
      window_stack_push(windows[2], true);
  }
  if(context == windows[2]){
      window_stack_push(windows[3], true);
  }
  if(context == windows[3]){
      window_stack_push(windows[4], true);
  }
  if(context == windows[4]){
    switch(status){
      case RESTING:
        if(rest_time < 0) procesa_fin_rest();
      break;
      case WORKING:
        if(series_time == 0) procesa_fin_serie();
      break;
    }
  }
}

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(context == windows[0]){
    series_num++;
  }
  if(context == windows[1]){
    series_time+=10;
  }
  if(context == windows[2]){
    rest_time+=10;
  }
  if(context == windows[4]){
    mode = (mode + 1 ) % 2;
  }

  draw_window(context);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(context == windows[0]){
    series_num = MAX(series_num-1,1);
  }
  if(context == windows[1]){
    series_time= MAX(series_time-10,0);;
  }
  if(context == windows[2]){
    rest_time= MAX(rest_time-10,-10);
  }
  if(context == windows[4]){
    mode = (mode + 1 ) % 2;
  }
  
  draw_window(context);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 1000, select_long_click_handler,select_long_click_handler);
}



static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  // Pantalla de seleccion de numero de series
  if(window == window_series_num){
    layer_series_num_title = text_layer_create(GRect(0,10,bounds.size.w, 30));
    text_layer_set_text_alignment(layer_series_num_title, GTextAlignmentCenter);
    text_layer_set_font(layer_series_num_title, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(layer_series_num_title));
    text_layer_set_text(layer_series_num_title,"#SERIES");
    
    layer_series_num = text_layer_create(GRect(0,60,bounds.size.w, 80));
    text_layer_set_text_alignment(layer_series_num, GTextAlignmentCenter);
    text_layer_set_font(layer_series_num, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(layer_series_num));
  }
  
  // Pantalla de seleccion de duracion de series
  if(window == window_series_time){
    layer_series_time_title = text_layer_create(GRect(0,10,bounds.size.w, 30));
    text_layer_set_text_alignment(layer_series_time_title, GTextAlignmentCenter);
    text_layer_set_font(layer_series_time_title, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(layer_series_time_title));
    text_layer_set_text(layer_series_time_title,"SERIES TIME");
    
    layer_series_time = text_layer_create(GRect(0,60,bounds.size.w, 80));
    text_layer_set_text_alignment(layer_series_time, GTextAlignmentCenter);
    text_layer_set_font(layer_series_time, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(layer_series_time));
  }

  // Pantalla de seleccion de duracion de descansos
  if(window == window_rest_time){
    layer_rest_time_title = text_layer_create(GRect(0,10,bounds.size.w, 30));
    text_layer_set_text_alignment(layer_rest_time_title, GTextAlignmentCenter);
    text_layer_set_font(layer_rest_time_title, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(layer_rest_time_title));
    text_layer_set_text(layer_rest_time_title,"REST TIME");
    
    layer_rest_time = text_layer_create(GRect(0,60,bounds.size.w, 90));
    text_layer_set_text_alignment(layer_rest_time, GTextAlignmentCenter);
    text_layer_set_font(layer_rest_time, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(layer_rest_time));
  }
  
 // Preworking Window
  if(window == window_work_pre){
    layer_work_pre = text_layer_create(GRect(0,0,bounds.size.w, 100));
    text_layer_set_text_alignment(layer_work_pre, GTextAlignmentCenter);
    text_layer_set_font(layer_work_pre, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(layer_work_pre));
  }
  
  // Working ScreWindowen
  if(window == window_work_progress){
    layer_work_title = text_layer_create(GRect(0,10,bounds.size.w, 120));
    text_layer_set_text_alignment(layer_work_title, GTextAlignmentCenter);
    text_layer_set_font(layer_work_title, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(layer_work_title));
    
    layer_work = text_layer_create(GRect(0,60,bounds.size.w, 100));
    text_layer_set_text_alignment(layer_work, GTextAlignmentCenter);
    text_layer_set_font(layer_work, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(layer_work));
    
    // Reset working vars to start values
    status = WORKING; 
    serie_actual = 0;
    rest_actual = 0;
    series_times = calloc(series_num,sizeof(int));
    rest_times = calloc(series_num,sizeof(int));
    for(int i = 0; i < series_num;i++){
      series_times[i] = 0;
    }  
    // Enable 1 sec timer and register callback
    timer = app_timer_register(1000, timer_callback, NULL);
  }
  
  // End&recap Window
  if(window == window_work_end){
    
    layer_work_end = text_layer_create(GRect(0,0,bounds.size.w, 2000));
    text_layer_set_text_alignment(layer_work_end, GTextAlignmentCenter);
    text_layer_set_font(layer_work_end, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    
    layer_work_end_scroll = scroll_layer_create(layer_get_frame(window_layer));
    scroll_layer_add_child(layer_work_end_scroll, text_layer_get_layer(layer_work_end));
    scroll_layer_set_click_config_onto_window(layer_work_end_scroll, window);

    layer_add_child(window_layer,scroll_layer_get_layer(layer_work_end_scroll));
      
    
    // Remove all windows from stack but first
    window_stack_remove(windows[1],false);
    window_stack_remove(windows[2],false);
    window_stack_remove(windows[3],false);
    window_stack_remove(windows[4],false);
  }

  draw_window(window);
}

static void window_unload(Window *window) {
  if(window == windows[0]){
    text_layer_destroy(layer_series_num_title);
    text_layer_destroy(layer_series_num);
  }
  if(window == windows[1]){
    text_layer_destroy(layer_series_time_title);
    text_layer_destroy(layer_series_time);
  }
  if(window == windows[2]){
    text_layer_destroy(layer_rest_time);
  }
  if(window == windows[3]){
    text_layer_destroy(layer_work_pre);
  }
  if(window == windows[4]){
    text_layer_destroy(layer_work_title);
    text_layer_destroy(layer_work);
  }
  if(window == windows[5]){
    text_layer_destroy(layer_work_end);
    scroll_layer_destroy(layer_work_end_scroll);
  }
}

static void init(void) {
  window_series_num = window_create();
  window_series_time = window_create();
  window_rest_time = window_create();
  window_work_pre = window_create();
  window_work_progress = window_create();
  window_work_end = window_create();
  windows[0]=window_series_num;
  windows[1]=window_series_time;
  windows[2]=window_rest_time;
  windows[3]=window_work_pre;
  windows[4]=window_work_progress;
  windows[5]=window_work_end;

  for(int i = 0; i < 6; i++){
    window_set_fullscreen(windows[i],true);
    window_set_click_config_provider(windows[i], click_config_provider);
    window_set_window_handlers(windows[i], (WindowHandlers) {.load = window_load, .unload = window_unload,});
  }
  window_stack_push(window_series_num, true);
}

static void deinit(void) {
  for(int i = 0; i < 6; i++){
    window_destroy(windows[i]);
  }
}


int main(void) {
  init();
  app_event_loop();
  deinit();
}