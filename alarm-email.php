<?php

$time = time();
$timestamp = date("H:i:s", $time);

$alarmch = $_POST["alarm"] . $_GET["alarm"];
$to = $_POST["toemail"] . $_GET["toemail"];
$from = $_POST["fromemail"] . $_GET["fromemail"];
$subject = "Alarm ALERT";
$message = "At: ".$timestamp." the Alarm status changed: ".$alarmch;
$headers = "From:".$from;

mail($to, $subject, $message, $headers);

print "Script Ran $time";

?>
