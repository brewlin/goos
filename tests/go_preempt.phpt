--TEST--
GOOS:  协程抢占
--FILE--
<?php
Runtime::GOMAXPROCS(1);
go(function(){
    sleep(1);
    echo "go 1\n";
});
go(function(){
    echo "go 2\n";
});
Runtime::wait();
?>
--EXPECT--
go 2
go 1

