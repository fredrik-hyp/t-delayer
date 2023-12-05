<?php
	require_once("dblib.php");
	
	function DBFetchStreaminterfaceLiveStatisticsByStreaminterfaceId($streaminterfaceid, $orderbypathnumber = false){
		global $DB;
		
		$order = "";
		if($orderbypathnumber === true)
		{
			$order = "ORDER BY pathnumber";
		}
		
		$sth = $DB->prepare("select * from streaminterfacelivestatistics where streaminterface_id=:streaminterfaceid " . $order);
		$sth->execute(array(":streaminterfaceid" => $streaminterfaceid));
		$result = $sth->fetchAll();
		if($result === false){
			return array();
		}
		return $result;		
	}
	
	function DBDeleteStreaminterfaceLiveStatisticsByStreaminterfaceId($streaminterfaceid)
	{
		global $DB;
		
		$sth = $DB->prepare("delete from streaminterfacelivestatistics where streaminterface_id=:streaminterfaceid");
		$result = $sth->execute(array(":streaminterfaceid" => $streaminterfaceid));

        if($result){
            return $sth->rowCount();
        }
        
        return -1;
	}
?>