<script type="text/javascript">
<!--
var maingraph;
var current_graph_id = '';
var debug_graph_edit = false;
var graphinfo= {}; 
var displayinfo = { start : 14*86400, cnt: '', end: '' };
var color_pool = Array('#33aa33','#4a00dc','#caac00','#3377cc','#00acfa','#cc3333')
var color_sel = 0;
var locked = true;

function graph_locked(warning) {		
  if(locked) {
    modal_warning("Graph Locked!", warning);
    return locked;
  }
  return locked;
}

function lock_forms() {
	$("input[name='lines']").attr("disabled", "true");
	$("input[name='math1']").attr("disabled", "true");
	$("input[name='math2']").attr("disabled", "true");
        $("input[name='view']").attr("disabled", "true");
        $("input[class='graphType']").attr("disabled", "true");
	$("select[name='derive']").attr("disabled", "true");
	$("select[class='av_stacks']").attr("disabled", "true");

	$("h2#graphTitle").unbind();
	$(".datatitle").unbind();
	$(".combinable").each( function() { $(this).draggable('destroy'); });
	$(".composite_function").each( function() { $(this).droppable('destroy'); });
}

function unlock_forms() {

	$("input[name='lines']").removeAttr("disabled");
	$("input[name='math1']").removeAttr("disabled");
	$("input[name='math2']").removeAttr("disabled");
        $("input[name='view']").removeAttr("disabled");
	//leave the graph type disabled, since we dont use it anywhere rightnow
	//$("input[class='graphType']").removeAttr("disabled");
	$("select[name='derive']").removeAttr("disabled");
	$("select[class='av_stacks']").removeAttr("disabled");

	$("h2#graphTitle").editable(function(value, settings) { 
           graphinfo.title = value;
           update_current_graph(false);
           return(value);
           }, { });

        $(".datatitle").editable(function(value, settings) {      
           graphinfo.datapoints[$(".datatitle").index(this)].name = value;
           update_current_graph(true);
           return(value);
        }, { }); 

	make_composite_draggables();
}

function make_composite_draggables() {

	$(".combinable").each ( function() { 
	    $(this).draggable({ cursor: 'move' });
	});

	$(".composite_function").each( function () { $(this).droppable({
	    drop: function(event, ui) { 
	         var data_index = $(".datatitle").index(ui.draggable);
 	         fbox = $(this).find("input[name='math2']");
		 fbox.val(fbox.val()+"["+data_index+"]");
		 var my_index = $("input[name='math2']").index(fbox);
		 graphinfo.datapoints[my_index].math2 = fbox.val();
 	         update_current_graph(true);
               }
          }); 
      });

}
function update_graph_permalink(id, start, end, gran){
 $("#gpermalink a").attr("href", "drawing_board.php?otype=graph&id="+id+"&start="+start+"&end="+end+"&gran="+gran);
}

function stop_streaming_graph (){
	 streaming = false;
	 $('#streambox').html('');
	 $(".stream-log").hide();
         $("#graph_datetool").show();
	 $("#play_pause_graph").html('PLAY');
	 $("#stopstream_graph").hide();
	 $("#osliderg").hide();	 
	 $(".stream_controls").width("");
}
//overlay mode: stackadd=false: we just add new stacks for any the incoming graph has
//stack mode: stackadd=trule, push new stacks onto matching stack indexes, else append
function aggregate_graph(id, stackadd) {
  if(!graph_locked("Click 'Edit Graph' to unlock.")){
	 if(id) $.getJSON("json/graph/info/" + id, function(g) {

	 	var max_old = (graphinfo.datapoints.length) ?  graphinfo.datapoints.length : 0;

	 	for(var i=0; i<g.datapoints.length; i++) {
			graphinfo.datapoints.push(g.datapoints[i]);
  		}  	 

		if(g.stacks) {
		  //we must use indexes for datapoints that are updated
		  for(var i=0; i<g.stacks.length; i++) {
		    for(var j=0; j<g.stacks[i].length; j++) {
		      g.stacks[i][j] += max_old;
		    }
		  }

		  if(stackadd) {
		    for(var i=0; i<g.stacks.length; i++){
		    	  if(graphinfo.stacks[i]) graphinfo.stacks[i] = graphinfo.stacks[i].concat(g.stacks[i]);
		 	  else graphinfo.stacks.push(g.stacks[i]);
		    }			  
		  }	
		  else {
		     for(var i=0; i<g.stacks.length; i++){
		     	   graphinfo.stacks.push(g.stacks[i]);
		     }
		  }
	       }
		update_current_graph(true);
		refresh_graph_from_json(graphinfo);
              });
  }
}

function set_current_graph_id(id) {
  stop_streaming_graph();
  $(".rememberGraph").fadeOut('fast');
  if(id!='')  {
    graphinfo.saved = true;
    locked = true;
    lock_forms();
    $(".editGraph").html('Edit Graph').fadeIn('slow');    
  }
  else {        
    locked = false;
    unlock_forms();    
    $(".editGraph").html('Editing!').fadeIn('slow');
  }
  current_graph_id = id;
  update_graph_permalink(current_graph_id, "", "", "");
  fetch_graph_info(current_graph_id);
}
var recurse = 0;
function update_current_graph(redraw, f) {
  if(recurse > 0) return;
  stop_streaming_graph();
  if(graphinfo.id) update_graph_permalink(current_graph_id, "", "", "");
  var str = JSON.stringify(graphinfo);
  if(debug_graph_edit) $("#payload").html(str);
  $.post("json/graph/store",
         {'json':str},
         function(d) {
           recurse++;
           graphinfo.id = current_graph_id = d.id;
           
           if(d.error) $("#gtool-error").html(d.error).fadeIn('fast');
           else $("#gtool-error").fadeOut('fast');      
           if(graphinfo.id && graphinfo.saved != true &&
              $(".rememberGraph:visible").length == 0) {
	      graphinfo.saved=false;
             $(".rememberGraph").html('"Remember" this graph.').fadeIn('slow');
             $(".rememberGraph").click(function() {
               graphinfo.saved = true;
	       if(graphinfo.stacks == undefined) graphinfo.stacks = Array();
               update_current_graph(false, function(r) {
                 if(r.error) { 
                   graphinfo.saved = false;
                    $("#gtool-error").html(r.error).fadeIn('fast');
                 }
                 else $(".rememberGraph").html('Remebered').fadeOut('slow');
               });
             });
           }
           if(redraw && maingraph) maingraph.ReconGraphRefresh({graphid: graphinfo.id, type: graphinfo.type});
           if(f) f(d);
           recurse--;
         }, 'json');
}

function graph_add_datapoint(d) {
  if(!graph_locked("Click 'Edit Graph' to unlock.")){
   for(i=0; i<graphinfo.datapoints.length; i++){
       if( (graphinfo.datapoints[i].sid==d.sid) && (graphinfo.datapoints[i].metric_name == d.metric_name) ) {
              modal_warning("Error", "Graphs cannot have duplicate datasets!");
	      return;
       }
    }
    if(d.axis == undefined) { d.axis = 'l'; }
    if(d.name == undefined) { d.name = d.target + '`' + d.metric_name; }
    graphinfo.datapoints.push(d);
    gtool_add_datapoint(d);
    update_current_graph(true);
  }
}
function gtool_add_composite() {
  var d ={};
  d.metric_type = 'composite';
  graphinfo.datapoints.push(d);
  gtool_add_datapoint(d);
  update_current_graph(true);
}
function gtool_add_guide() {
  var d = {};
  d.metric_type = 'guide';
  graphinfo.datapoints.push(d);
  gtool_add_datapoint(d);
  update_current_graph(true);
}

function gtool_add_datapoint(d) {
  var o;
  if(d.metric_type == 'guide') {
    o = $("#guideeditor").clone();
  }
  else if(d.metric_type == 'composite') {
    o = $("#compositeeditor").clone();
  }
  else if(d.metric_type == 'text') {
    o = $("#textdatapointeditor").clone();
  }
  else {
    o = $("#datapointeditor").clone();
  }
  recurse++;
 
  if(d.color == null) {
    d.color = color_pool[color_sel];
    color_sel = (color_sel + 1) % color_pool.length;
  }
  o.find('.colorPickerHolder').ColorPicker({
    flat: true,
    color: '#ffffff',
    onSubmit: (function(dpo,dp) {
      var cs = dpo.find('.colorSelector');
      return function(hsb, hex, rgb) {
        dp.color = '#' + hex;
        cs.css('border', '1px solid ' + dp.color);
        cs.find('div').css('backgroundColor', dp.color)
                      .css('opacity', 0.2);
        cs.parent().find('.colorPickerHolder').stop().animate({height:0},500);
        update_current_graph(true);
      }
    })(o,d)
  });
  o.find('.colorPickerHolder > div').css('position', 'absolute');
  o.find('.colorSelector').css('border', '1px solid ' + d.color);
  o.find('.colorSelector div').css('backgroundColor', d.color)
                              .css('opacity',
                                   (d.opacity == null) ? 0.2 : d.opacity);
  o.find('.colorSelector').bind('click', function() {
    if(!locked){
        var activecp = $(this).parent().find('.colorPickerHolder');
	var h = activecp.height();
        $("#gtool #dataPoints .colorPickerHolder").filter(function(index) {
             return ($(this) != activecp);
        }).stop().animate({height:0}, 0);;
        $(this).parent().find('.colorPickerHolder').stop().animate({height: h != 0 ? 0 : 173}, 500);
    }
  });

  o.find(d.axis == "l" ? ".axisl" : ".axisr").addClass("axison");
  o.find("span.axis").click(function(){
    if(!locked) {
       $(this).siblings().removeClass("axison");
       $(this).addClass("axison");
       d.axis = $(this).hasClass("axisl") ? "l" : "r";
       update_current_graph(true);
    }
  });

  if(!d.hidden){o.find('input[name="view"]').attr("checked","checked");}
  o.find('input[name="view"]').change(function(){
    d.hidden = !($(this).attr("checked"));
    update_current_graph(true);
  }).change();

  o.find('select[id="datastack"]').change(function(){
      var si = find_in_stacks($(".av_stacks").index(this));
       if(si != -1) graphinfo.stacks[si.i].splice(si.j, 1);

       if(graphinfo.stacks[ $(this).val() ]) {
         graphinfo.stacks[ $(this).val()].push($(".av_stacks").index(this));
	 graphinfo.stacks[ $(this).val()].sort(function(a,b){return a - b});
       } 
       update_current_graph(true);

      $(".graphStacks").empty();

      for(var i=0; i<graphinfo.stacks.length; i++) {
        $(".graphStacks").append('<br>Stack '+i+': ');
        for(var j=0; j<graphinfo.stacks[i].length; j++){
            $(".graphStacks").append(graphinfo.datapoints[graphinfo.stacks[i][j]].name + ",");
        }
      }
    }).change();


  o.find('.deletedatapoint').click(function(){
    if(!locked){
       for(var i=0; i<graphinfo.datapoints.length; i++) {
         if(graphinfo.datapoints[i] == d) {
           graphinfo.datapoints.splice(i,1);
	   var si = find_in_stacks(i);
	   if(si != -1) graphinfo.stacks[si.i].splice(si.j, 1);

	   //we must decrement the datapoint indexes in the stack array as well
	   for(var k=0; k<graphinfo.stacks.length; k++) {
	     for(var j=0; j<graphinfo.stacks[k].length; j++) {
               if(graphinfo.stacks[k][j]>i) graphinfo.stacks[k][j]-=1;
	     }
	   }	   
	   //TODO do we wanna decrement the indexes referred in composite functions for the user?
           break;
         }
       }          
          update_current_graph(true);
     	  refresh_graph_from_json(graphinfo);
       }
    return false;
  });

  if(d.metric_type == 'text') {
//    o.find('tr.mathbox').remove();
    o.find('select[name="derive"]').val('false');
    o.find('input[name="lines"]').attr('checked', d.math1=='lines');
    o.find('input[name="lines"]').change(function(){
      d.math1 = $(this).is(':checked') ? 'lines':'' ;
      update_current_graph(true);
    });
  }
  else {
    if(d.derive){
      o.find('select[name="derive"]').val(d.derive);
    }
    o.find('select[name="derive"]').change(function(){
      d.derive = $(this).find(":selected").val();
      update_current_graph(true);
    }).change();

    o.find('input[name="math1"]').val(d.math1);
    o.find('input[name="math1"]').change(function(){
      d.math1 = $(this).val();
      update_current_graph(true);
    });
    o.find('input[name="math2"]').val(d.math2);
    o.find('input[name="math2"]').change(function(){
      d.math2 = $(this).val();
      update_current_graph(true);
    });
  }

  d.mouse_title = ( (d.target!=undefined) ? d.target+"`": '') +
  		    	( (d.module!=undefined) ? d.module+"`": '') +
			( (d.orig_name!=undefined)? d.orig_name+"`": '') +
			( (d.metric_name!=undefined)? "`"+d.metric_name: '');
  			
  o.find(".datatitle").html(d.name);
  o.find(".datatitle").attr("title", d.mouse_title);

  if (!locked) {
      o.find(".datatitle").editable(function(value, settings) {
      graphinfo.datapoints[$(".datatitle").index(this)].name = value;
      update_current_graph(true);
      return(value);
    }, { });
 }

  recurse--;
  $("#gtool #dataPoints").append(o.children());    
  if(!locked){ make_composite_draggables();}
}

function find_in_stacks(dnum) {
  for(var i=0; i<graphinfo.stacks.length; i++) {
      for(var j=0; j<graphinfo.stacks[i].length; j++) {
         if(graphinfo.stacks[i][j] == dnum) {            
            var r = { i: i, j: j}; return r;
	 }        
      }
  }
  return -1;
}
//this function works on the maingraph dom element, which is used througout in graph editing
function refresh_graph_from_json(j) {
  graphinfo = j;
  if(graphinfo.stacks == undefined) graphinfo.stacks = Array();
  if(graphinfo.datapoints == undefined) graphinfo.datapoints = Array();
  $("h2#graphTitle").html(graphinfo.title ? graphinfo.title : 'Graph Title (click to edit)');

  $("input[name='graphtype']:checked").removeAttr("checked");
  $("input[name='graphtype']")
    .filter(function(i){return $(this).val() == graphinfo.type;})
    .attr("checked","checked");
  $("#dataPoints").empty();

  for(var i=0; i<graphinfo.datapoints.length; i++) {
    gtool_add_datapoint(graphinfo.datapoints[i]);
  }

  $(".graphStacks").empty();
  $('.av_stacks').find('option').remove().end().append('<option value="-1">select </option>')

  for(var i=0; i<graphinfo.stacks.length; i++) {
    $(".graphStacks").append('<br>Stack '+i+': ');	
    $('.av_stacks').append('<option value="'+i+'">Stack '+i+'</option>');

    for(var j=0; j<graphinfo.stacks[i].length; j++){
        $(".graphStacks").append(graphinfo.datapoints[graphinfo.stacks[i][j]].name + ",");
    }
  }

  $('select[id="datastack"]').each( function()  {    
    var si = find_in_stacks($(".av_stacks").index(this));
    if(si != -1) $(this).val(si.i);
  });

  if(maingraph) {
    if(graphinfo.id) 
      maingraph.ReconGraphRefresh({graphid: graphinfo.id, type: graphinfo.type, stacks: graphinfo.stacks});
    else
      maingraph.ReconGraphReset();
  }
}
function fetch_graph_info(id) {
  graphinfo.id = id;
  if(id) $.getJSON("json/graph/info/" + id, refresh_graph_from_json);
  else refresh_graph_from_json({});
}
-->
</script>
<!--<p><a href="">username</a> / <a href="#">worksheet</a></p>-->
<p/>
<span class="rememberGraph"></span>
<span class="blankGraph">New Blank</span>
<span class="exportcsv" id="exportcsvg"><a>CSV</a></span>
<span class="exportcsv" id="exportpngg" onclick="exportCanvasAsPNG('#maingraph')">PNG</span>
<span class="stream_controls">
<span id="play_pause_graph">PLAY</span>
<span id="stopstream_graph">STOP</span>
<div id="osliderg" style="display:none">
<div id="pollsliderg"></div>
<div id="polltimeg"></div>
</div>
</span>
<span class="editGraph">Edit Graph</span>
<span class="permalink" id="gpermalink"><a href="">Link</a></span>

<h2 id="graphTitle"></h2>
<!-- date range box -->
<script type="text/javascript">
$(document).ready(function(){

	lock_forms();

        var time_windows = { '2d' : 86400*2,
                              '1w' : 86400*7,
                              '2w' : 86400*14,
                              '4w' : 86400*28,
                              '1y' : 86400*365,
                            };
        var state = false;

	//only allow graph editing if not streaming
	$(".editGraph").click(function() {	
	    if(!streaming){
	        if(locked){
		    locked = false;		    
		    unlock_forms();
		    $(".editGraph").html('Editing!').fadeIn('slow');
		}
		else if(!locked){
		    locked = true;	
		    lock_forms();
                    $(".editGraph").html('Edit Graph').fadeIn('slow');
		}
	    }
	});

	$(".editStacks").click(function() {
	       if(!graph_locked("Unlock by clicking 'Edit Graph'.") && graphinfo.id) {           	       
     	         graphinfo.stacks.push([]);
		 var ns = graphinfo.stacks.length - 1;
	         $(".graphStacks").append('<p>Stack '+ns+'</p>');
		 $('.av_stacks').append('<option value="'+ns+'">Stack '+ns+'</option>');
  		 update_current_graph(true);
	       }
       });	
       //only allow stream toggling if the graph is locked and saved
       $("#play_pause_graph").click(function() {
	 if(locked && graphinfo.saved) {
      	  if($(this).html() == 'PLAY') {
            $(this).html('PAUSE');
	    //if we are playing for the frist time
	    if(!streaming) {
	      $('#graph_datetool').hide();
	      $(".stream-log").show().html("stream log_");
	      $("#stopstream_graph").show();
	      $("#osliderg").show();
	      //theres probably a better way to make sure stuff fits in the stream_controls div
	      $(".stream_controls").width("290px");
	    }
	    //setup/restart the plotting
	    stream_data(graphinfo.id, maingraph, $('#streambox'));
          }
          else if($(this).html() == 'PAUSE') {
	     $(this).html('PLAY');
	     //this is where we pause for a bit
	     maingraph.stopTime();
          }
        }
       });

       $("#stopstream_graph").click(function() {
        stop_streaming_graph();
        update_current_graph(true);
       });

       $("#polltimeg").html(polltime+" ms");

       $("#pollsliderg").slider({orientation: 'horizontal', value: polltime, max: 10000, min: 1000, step: 1000,
	    change: function (event, ui) {  
                       polltime = $(this).slider('option', 'value'); 
		$("#polltimeg").html(polltime+" ms");
	        if(streaming) {
		    streaming = false;
		    $('#streambox').html('');
		    $("#play_pause_graph").html('PAUSE');
		    stream_data(graphinfo.id, maingraph, $('#streambox'));
		}
	      }			
        });

        $("#graph_datetool .btn-slide").click(function(){
                $("#graph_widgetCalendar").stop().animate({
                     height: state ? 0 :
                       $('#graph_widgetCalendar div.datepicker').get(0).offsetHeight
                  }, 500);
                state = !state;
                $(this).toggleClass("active");
                return false;
        });
        $("#graph_datetool .datechoice").click(function(){
                $("#graph_datetool .range a.btn-slide").html("YYYY/MM/DD - YYYY/MM/DD");
                $("#graph_widgetCalendar").slideUp("slow");
                $("#graph_datetool .datechoice").removeClass("selected");

		//start here will be in seconds
                displayinfo.start = time_windows[$(this).html()];
                displayinfo.end = '';
                $(this).addClass("selected");
		var cdate = new Date();
		update_graph_permalink(graphinfo.id, parseInt(cdate.getTime() - displayinfo.start*1000), "", "");
                maingraph.ReconGraphRefresh({graphid: graphinfo.id, start: time_windows[$(this).html()], end: '', type: graphinfo.type});
                return false;
        });
        $('#graph_widgetCalendar').DatePicker({
                flat: true,
                format: 'Y/m/d',
                date: [new Date(), new Date()],
                calendars: 3,
                mode: 'range',
                starts: 1,
                onChange: function(formated) {
                        var dates;
                        dates = formated[0].split('/');
                        var start = new Date(dates[0], dates[1]-1, dates[2], 0, 0, 0);
                        dates = formated[1].split('/');
                        var end = new Date((new Date(dates[0], dates[1]-1, dates[2], 0, 0, 0)).getTime() + 86400000);
                        displayinfo.start = start.toUTCString();
                        displayinfo.end = end.toUTCString();
			update_graph_permalink(graphinfo.id, parseInt(start.getTime()), parseInt(end.getTime()), "");
                        maingraph.ReconGraphRefresh({graphid: graphinfo.id, start: displayinfo.start, end: displayinfo.end, type: graphinfo.type});
                        $('#graph_datetool .range a.btn-slide').get(0).innerHTML = formated.join(' - ');
                }
        });
        $(".graphType").change(function(){
                graphinfo.type = $(this).val();
		update_current_graph(true);
        });
        $("#gtool-error").click(function(){
          $("#gtool-error").fadeOut("slow");
        });
        $(".addGuide").click(function() {
	  if(!locked){
            gtool_add_guide();
	  }
        });
	$(".addComposite").click(function() {
	  if(!locked){
            gtool_add_composite();
	  }
        })

        $("span.blankGraph").click(function() {

	  stop_streaming_graph();

          // current graph is saved, so just give a new one.
          if(graphinfo.saved) set_current_graph_id('');

          // otherwise make sure the user wants to move on, and if so, give a new graph and orphan the unsaved one.
          else if(graphinfo.saved!=undefined && graphinfo.saved==false) {
            confirm("I will forget the current graph.  Are you sure?", function() {
              set_current_graph_id('');
	    });
          }

          return false;
        });

        set_current_graph_id('');
        maingraph = $('<div></div>').attr("id", "maingraph").ReconGraph({graphid: graphinfo.id, width: 780, height: 400});
        $("#maingraphHolder").append(maingraph);
});
</script>

<div id="graph_datetool">
	<div class="zoom">
		<dl>
			<dt>Zoom:</dt>
			<dd><a href="#" class="first datechoice">2d</a></dd>
			<dd><a href="#" class="datechoice">1w</a></dd>
			<dd><a href="#" class="selected datechoice">2w</a></dd>
			<dd><a href="#" class="datechoice">4w</a></dd>
			<dd><a href="#" class="datechoice">1y</a></dd>
		</dl>
	</div>
	<div class="range">
		<dl>
			<dt>Date Range:</dt>
			<dd><a href="" class="btn-slide">YYYY/MM/DD - YYYY/MM/DD</a></dd>
		</dl>
	</div>
<br/>
	<div id="graph_widgetCalendar" class="widgetCalendar"></div>
</div>
			
<!-- confirm box -->
<div id="confirm" style="display:none">
	<a href="#" title="Close" class="modalCloseX modalClose">x</a>
	<div class="header"><span>Confirm</span></div>
	<p class="message"></p>
	<div class="buttons">
		<div class="no modalClose">No</div><div class="yes">Yes</div>
	</div>
</div>

<div>
	<div id="maingraphHolder">
        </div>
        <div class='stream-log' style='display:none'></div>
	<form action="#" name="form4" id="form4" style="margin:1em 0;text-align:center;">
	<fieldset>
	<legend style="display:none;">View</legend>
	<label for="std_view"><input class="graphType" type="radio" name="graphtype" id="std_view" value="standard"/> Standard View</label> &nbsp;&nbsp;&nbsp;
	</fieldset>
	</form>
	
</div>
<span class="graphStacks"></span>
<div><span class="editStacks">Add Stack Set</span></div>
<div style="float:right"><span class="addGuide">Add Guide</span></div>
<div style="float:right"><span class="addComposite">Add Composite</span></div>
<br style="clear:right" />
<div class="error"><p class="error" id="gtool-error"></p></div>
<table id="gtool">
	<tr>
		<th></th>
		
		<th class="data">Data Points</th>
		<th>Stacking</th>
		<th>Color</th>
		<th>Derivative</th>
		<th>Axis</th>
		<th></th>
		<th></th>
	</tr>
	<tbody id="dataPoints">
	</tbody>
</table>

<div style="display:none">
<form id="hiddeneditor">
	<table>
	<tbody id="guideeditor">
	<tr>	        
		<td><input name="view" type="checkbox" value="1" /></td>
		<td class="data datatitle"></td>
		<td>&nbsp;</td>
		<td><div class="colorPicker"><div class="colorSelector"><div style="background-color: #fff"></div></div><div class="colorPickerHolder"></div></div></td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td><a href="#" class="deletedatapoint"><span>delete</span></a></td>
		<td class="math">math</td>
	</tr>
	<tr class="mathbox">
		<td colspan="7">
		<div>
			<label for="math">Display Math</label> <input name="math1" type="text" value="" style="width:380px;" />
		</div>
		<div>
			<label for="math">Percentile</label> <input name="math2" type="text" value="" style="width:380px;" />
		</div>
		</td>
	</tr>
	</tbody>
	<tbody id="compositeeditor">
	<tr>	        
		<td><input name="view" type="checkbox" value="1" /></td>
		<td class="data datatitle"></td>
		<td>&nbsp;</td>	
		<td><div class="colorPicker"><div class="colorSelector"><div style="background-color: #fff"></div></div><div class="colorPickerHolder"></div></div></td>
		<td>&nbsp;</td>	
		<td><span class="axis axisl"> L</span> <span class="axis axisr"> R</span></td>
		<td><a href="#" class="deletedatapoint"><span>delete</span></a></td>
		<td class="math">math</td>
	</tr>
	<tr class="mathbox">
		<td colspan="7">
		<div>
			<label for="math">Display Math</label> <input name="math1" type="text" value="" style="width:380px;" />
		</div>
		<div class="composite_function">
			<label for="math">Function</label> <input name="math2" type="text" value="" style="width:380px;" />
		</div>
		</td>
	</tr>
	</tbody>
	<tbody id="datapointeditor">
	<tr>
		<td><input name="view" type="checkbox" value="1"/></td>
		<td class="data datatitle combinable"></td>
		<td><select id="datastack" class="av_stacks"><option value='na'>select</option></select></td>
		<td><div class="colorPicker"><div class="colorSelector"><div style="background-color: #fff"></div></div><div class="colorPickerHolder"></div></div></td>
		<td><select name="derive"><option value="false">no</option><option value="derive">derive</option><option value="counter">counter</option></select></td>
		<td><span class="axis axisl"> L</span> <span class="axis axisr"> R</span></td>
		<td><a href="#" class="deletedatapoint"><span>delete</span></a></td>
		<td class="math">math</td>
	</tr>
	<tr class="mathbox">
		<td colspan="7">
		<div>
			<label for="math">Display Math</label> <input name="math1" type="text" value="" style="width:380px;" />
		</div>
		<div>	
			<label for="math">Source Math</label> <input name="math2" type="text" value="" style="width:380px;" />
		</div>
		</td>
	</tr>
	</tbody>
	<tbody id="textdatapointeditor">
	<tr>
		<td><input name="view" type="checkbox" value="1"/></td>
		<td class="data datatitle"></td>
		<td>&nbsp;</td>
		<td><div class="colorPicker"><div class="colorSelector"><div style="background-color: #fff"></div></div><div class="colorPickerHolder"></div></div></td>
		<td>&nbsp;</td>
		<td><span class="axis axisl"> L</span> <span class="axis axisr"> R</span></td>
		<td><a href="#" class="deletedatapoint"><span>delete</span></a></td>
	</tr>
	<tr class="mathbox">
		<td colspan="7">
		<div>
			<label for="math">Draw vertical lines</label> <input type="checkbox" name="lines" />
		</div>
		</td>
	</tr>
	</tbody>
        </table>
</form>
</div>

<div id="payload">
</div>
