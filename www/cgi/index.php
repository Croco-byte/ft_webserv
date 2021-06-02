<?php
header("test: abcdef");

var_dump($_SERVER);

echo "POST : ";
var_dump($_POST);

echo "GET : ";
var_dump($_GET);

echo "QUERY_STRING => " . $_SERVER["QUERY_STRING"] . "\n" ;
echo "CONTENT_LENGTH => " . $_SERVER["CONTENT_LENGTH"] . "\n" ;
echo "CONTENT_TYPE => " . $_SERVER["CONTENT_TYPE"] . "\n" ;

?>
