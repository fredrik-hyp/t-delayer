<?php
		// -- Copyright Polylegio AB 2012	
		// -- All rights reserved

		// --- @provides PostgreSQL IMS deviceactions functions
		// --- @author Fredrik Nilsson <fredrik.nilsson@polylegio.se>
		// --- @last-modified 2012-03-29

	
	require_once("dblib.php");
	$dbonn = DBConnect();

	function DBFetchDeviceactionById($id){
		if(!is_numeric($id)){
			return array();
		}
		$result = DBFetch("deviceactions", "*", "id=$id", "");
		return DBSingleArr($result);
	}

	function DBFetchDeviceactionsByDeviceId($deviceid){
		if(!is_numeric($deviceid)){
			return -1;
		}
		
		$deviceid = DBEscapeString($deviceid);
		$result = DBFetch("deviceactions", "*", "device_id='$deviceid'", "");

		return $result;
	}
	
	function DBFetchAllDeviceactions(){
		$result = DBFetch("deviceactions", "*", "", "name");
		return $result;
	}

	function DBFetchDeviceactionsByAction($action){
		$action = DBEscapeString($action);
		$result = DBFetch("deviceactions", "*", "action='$action'", "");
		return $result;
	}
	
	function DBFetchDeviceActionByDeviceIdAndAction($deviceid, $action){
		if(!is_numeric($deviceid)){
			return -1;
		}
		$deviceid = DBEscapeString($deviceid);
		$action = DBEscapeString($action);
		
		$result = DBFetch("deviceactions", "*", "device_id=$deviceid AND action='$action'","");
		return DBSingleArr($result);
	}
	
	function DBUpdateDeviceaction($id, $deviceid, $action){
		if(!is_numeric($id)){
			return -1;
		}
		if(!is_numeric($deviceid)){
			return -1;
		}
		
		$id = DBEscapeString($id);
		$deviceid = DBEscapeString($deviceid);
		$action = DBEscapeString($action);

		$result = DBUpdate("deviceactions", "device_id=$deviceid, action='$action'", "id=$id");
		return $result;
	}


	function DBAddDeviceaction($deviceid, $action){
		if(!is_numeric($deviceid)){
			return -1;
		}
		
		$deviceid= DBEscapeString($deviceid);
		$action = DBEscapeString($action);
		
		$result = DBInsertAndGetID("deviceactions", "device_id, action", "$deviceid, '$action'");		
		return $result;
	}
	
	function DBDeleteDeviceaction($id){
		if(!is_numeric($id)){
			return -1;
		}
		
		$id = DBEscapeString($id);
		$result = DBDelete("deviceactions", "id=$id");
		return $result;
	}
?>