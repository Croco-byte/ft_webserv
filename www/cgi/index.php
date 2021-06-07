<?php

header("test: abcdef");

echo '<script src="https://code.jquery.com/jquery-3.6.0.min.js" integrity="sha256-/xUj+3OJU5yExlq6GSYGSHk7tPXikynS7ogEvDej/m4=" crossorigin="anonymous"></script>'; 
var_dump($_SERVER);

echo "POST : ";
var_dump($_POST);

echo "GET : ";
var_dump($_GET);

echo "QUERY_STRING => " . $_SERVER["QUERY_STRING"] . "\n" ;
echo "CONTENT_LENGTH => " . $_SERVER["CONTENT_LENGTH"] . "\n" ;
echo "CONTENT_TYPE => " . $_SERVER["CONTENT_TYPE"] . "\n" ;

?>
