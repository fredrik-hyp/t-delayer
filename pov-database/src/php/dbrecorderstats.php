<?php
	require_once("dblib.php");
	
	function DBFetchRecorderstatsByStreaminterfaceId($streaminterfaceid)
    {
		global $DB;
		$sth = $DB->prepare("SELECT * FROM recorderstats WHERE streaminterface_id=:streaminterfaceid");
		$sth->execute(array(":streaminterfaceid" => $streaminterfaceid));
		$result = $sth->fetchAll();
		if($result === false)
        {
			return array();
		}
		return $result;
	}
?>
