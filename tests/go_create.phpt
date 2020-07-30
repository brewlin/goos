--TEST--
GOOS:  协程创建
--FILE--
<?php
Runtime::GOMAXPROCS(10);
for($i = 0; $i < 10; $i ++){
    go(function()use($i){
        echo "-";
    });
}
Runtime::wait();
?>
--EXPECT--
----------
