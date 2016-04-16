<?php
$file = 'xml.xml';
$content = file_get_contents($file);

$ret = xml2array($content);
var_dump($ret);
