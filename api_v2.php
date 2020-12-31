<?php 
/**
   Remote Friendship Lamp
   23.11.2020 
   Last update: 
      - 31.12.2020
      - using board id as identifier (so each lamp can run with the exact same code)
      - using WiFiManager library for accessing unknown wifis (so each lamp can run w... you get it)
      - reading pairs from a text file
   Jürgen Krauß
   @MirUnauffaellig
**/

/* Possible GET parameters and values (that's maybe an unnecessary layer of security but gives you a good feeling for which values to expect in your script) */
$valid = array(
	"mode" => array(true, array("get-status", "set-status", "admin")),
	"identifier" => array(false),
	"key" => array(true, array("AUTH_KEY")),
	"status" => array(false)
);
$colors = array("i_think" => "9,33,211", 
				"you_think" => "255,102,204", 
				"we_think" => "102,255,51");

/* Check existence and plausibility of GET parameters AND copy them into local variables for easier referencing */
foreach ($valid as $key => $v):
	if ($v[0]):
		if (!(isset($_GET[$key])) || (!in_array($_GET[$key], $v[1]))):
			die("Wrong or no parameters."); 
		endif;
		$$key = $_GET[$key];
	else:
		if (isset($_GET[$key])) $$key = $_GET[$key];
	endif; 
endforeach;

/* Check for pairs in a textfile – pretty sure, this could be improved. */
$error = "";
$otherfile = "";
$pairs = file("pairs.txt");
if (count($pairs) == 0 || trim($pairs[0]) == ""):
	$error = '<p><strong>Error:</strong> No pairs specified.</p>';
else:
	foreach ($pairs as $pair):
		$pair = explode(" ", $pair);
		if (count($pair) != 2) continue;
		if ($identifier != $pair[0] && $identifier != $pair[1]): 
			continue; 
		else:
			$otherfile = ($identifier == $pair[0] ? $pair[1] : $pair[0]).".txt";
		endif;
	endforeach;
	if ($otherfile == "") $error = '<p><strong>Error:</strong> No pair found for ID '.$identifier.".</p>";
endif;

/* Read status file(s) (or create it) */
$statusfile = $identifier.".txt";
//if ($identifier == $valid['identifier'][1][0]) $otherfile = $valid['identifier'][1][1].".txt"; else $otherfile = $valid['identifier'][1][0].".txt"; 

if (!file_exists($statusfile)) file_put_contents($statusfile, "0,0,0");
if (!file_exists($otherfile)) file_put_contents($otherfile, "0,0,0");

$last_modified = date ("d.m.Y H:i:s", filemtime($statusfile));

/* Mode switcher */
switch($mode) { 
	/* getting the status */
	case "get-status":
		if ($error != "") die($error);
		$diff = round((time()-filemtime($statusfile))/60);
		$diff_other = round((time()-filemtime($otherfile))/60);

		// If file older than 15 minutes, reset it
		if ($diff > 15 && $diff_other > 15) {
			file_put_contents($statusfile, "0,0,0");
			file_put_contents($otherfile, "0,0,0");
		}

		$fc = file_get_contents($statusfile);
		$fc = explode(",", $fc);
		if (count($fc) != 3) { 
			echo "0,0,0";
		} else { 
			$i = 0;
			foreach ($fc as $f) {
				if (!is_integer($f*1)) {
					echo "0";
				} elseif ($f >= 255) {
					echo "255";
				} elseif ($f < 0) {
					echo "0";
				} else {
					echo $f;
				}
				if (++$i <= 2) echo ",";
			}
		}
	break;

	/* setting the status */
 	case "set-status":
 		if ($error != "") die($error);
		if (file_get_contents($statusfile) == "0,0,0" || file_get_contents($statusfile) == $colors['i_think']) { 
 			file_put_contents($statusfile, $colors['i_think']);
 			echo $colors['i_think'];
 			file_put_contents($otherfile, $colors['you_think']);
 		} elseif (file_get_contents($statusfile) == $colors['you_think'] || file_get_contents($statusfile) == $colors['we_think']) {
 			file_put_contents($statusfile, $colors['we_think']);
 			echo $colors['we_think'];
 			file_put_contents($otherfile, $colors['we_think']);
 		}
	break;

	/* debugging info */
	case "admin": 
		echo "<pre style=\"font-size: 1.5em;\">";
		echo $error;
		$txtfiles = (listfiles("./", "txt"));
		if (count($txtfiles) > 0) {
			foreach ($txtfiles as $t) {
				echo "<p>";
				$diff = round((time()-filemtime($t))/60). " minutes";
				echo "<strong>".$t."</strong> | file age: ".date ("d.m.Y H:i:s", filemtime($t))." | ".$diff."\n";
				echo file_get_contents("./".$t)."";
				echo "</p>";
			}
		}
		echo "<br />";
		echo "<p>";
		var_dump($valid);
		echo "</p>";
		echo "</pre>";
	break;
}	

function listfiles($dir, $filter_ext) {
	$return = array();
	if ($handle = opendir($dir)) {
	    while (false !== ($entry = readdir($handle))) {
	    	if ($entry != "." && $entry != ".." && ".".$filter_ext == substr($entry, -1*(strlen($filter_ext)+1)) ) {
	            $return[] = $entry;
	        }
	    }
	    closedir($handle);
	}
	return $return;
}
?>
