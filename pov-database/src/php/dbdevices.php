<?php
		// -- Copyright Polylegio AB 2012	
		// -- All rights reserved

		// --- @provides PostgreSQL IMS devices functions
		// --- @author Fredrik Nilsson <fredrik.nilsson@polylegio.se>
		// --- @last-modified 2012-03-29

	
	require_once("dblib.php");
	require_once("dborganisations.php");
	$dbonn = DBConnect();


	function DBFetchDevices(){
		global $DB;
		$sth = $DB->prepare("SELECT * FROM devices ORDER BY name");
		$result = $sth->execute();
		if($result){
			return $sth->fetchAll();
		}
		return array();
	}
	
	function DBFetchDeviceById($id){
		if(!is_numeric($id)){
			return array();
		}
		$result = DBFetch("devices", "*,to_char(last_heard_of,'YY-MM-DD HH24:MI:ss') as last_checkin", "id=$id", "");
		return DBSingleArr($result);
	}

	function DBFetchDeviceByIdAndUserId($id, $userid){
		if(!is_numeric($id)){
			return array();
		}
		
		if(!is_numeric($userid)){
			return array();
		}
		
		$organisations = DBFetchOrganisationsWithUserId($userid);
		if(sizeof($organisations) < 1){
			return array();
		}
		
		$orgconds = "(";
		foreach($organisations as $organisation){
			$orgconds .= " organisatioanddevice.organisation_id=" . $organisation['id'] . " OR";
		}
		
		$orgconds = substr($orgconds, -2);
		$orgconds .=")";
		
		$result = DBFetch("devices,organisationanddevice", "devices.*,to_char(last_heard_of,'YY-MM-DD HH24:MI:ss') as last_checkin", "organisationanddevice.device_id=devices.id AND devices.id=$id AND $orgconds");
		return $result;
	}
	
	function DBFetchApprovedDevices(){
		global $DB;
		$sth = $DB->prepare("select *,to_char(last_heard_of,'YY-MM-DD HH24:MI:ss') as last_checkin  from devices where approved='t'");
		$result = $sth->execute();
		if($result){
			return $sth->fetchAll();
		}
		return array();
	}

	function DBFetchNonApprovedDevices(){
		global $DB;
		$sth = $DB->prepare("select *,to_char(last_heard_of,'YY-MM-DD HH24:MI:ss') as last_checkin from devices where approved='f'");
		$result = $sth->execute();
		if($result){
			return $sth->fetchAll();
		}
		
		return array();
	}
	
	function DBFetchDeviceByName($name){
		$name = DBEscapeString($name);
		$result = DBFetch("devices", "*,to_char(last_heard_of,'YY-MM-DD HH24:MI:ss') as last_checkin", "name='$name'", "");

		return DBSingleArr($result);
	}

	function DBFetchDeviceBySerialnumber($serialnumber){
		if(!is_numeric($serialnumber)){
			return -1;
		}
		
		$serialnumber = DBEscapeString($serialnumber);
		$result = DBFetch("devices", "*,to_char(last_heard_of,'YY-MM-DD HH24:MI:ss') as last_checkin ", "serialnumber=$serialnumber", "");

		return DBSingleArr($result);	
	}
	
	function DBFetchAllDevices(){
		$result=DBFetch("devices", "*,to_char(last_heard_of,'YY-MM-DD HH24:MI:ss') as last_checkin ", "", "name");
		return $result;
	}

	function DBFetchDevicesForUser($userid){
		if(!is_numeric($userid)){
			return array();
		}
		
		$userid = DBEscapeString($userid);
		$result = DBFetch("useranddevice, devices", "devices.*", "useranddevice.device_id=devices.id AND useranddevice.user_id=$userid", "devices.name");
		return $result;
		
	}

	function DBFetchDevicesWithOrganisationId($organisationid){
		if(!is_numeric($organisationid)){
			return array();
		}
		$organisationid = DBEscapeString($organisationid);
		$result = DBFetch("organisationanddevice, devices", "devices.* ,to_char(last_heard_of,'YY-MM-DD HH24:MI:ss') as last_checkin ", "organisationanddevice.device_id=devices.id AND organisationanddevice.organisation_id=$organisationid", "name");
		return $result;
	}
	
	function DBFetchDevicesWithoutOrganisation(){
		global $DB;
		$alldevices = DBFetchDevices();
		$sth_orgdevices =  $DB->prepare("SELECT DISTINCT devices.* from devices,organisationanddevice WHERE organisationanddevice.device_id=devices.id");
		$result = $sth_orgdevices->execute();
		if(!$result){
			return array();
		}
		
		$devices = DBRemoveFromList($alldevices, $sth_orgdevices->fetchAll(), "id");
		return $devices;
	}
	/*
	function DBFetchDeviceByIdAndUserId($id, $userid){
		if(!is_numeric($id)){
			return array();
		}
		if(!is_numeric($userid)){
			return array();
		}
		
		$id = DBEscapeString($id);
		$userid = DBEscapeString($userid);
		
		$result = DBFetch("useranddevice, devices", "devices.*,to_char(last_heard_of,'YY-MM-DD HH24:MI:ss') as last_checkin ", "useranddevice.device_id=devices.id AND useranddevice.user_id=$userid AND devices.id='$id'", "");
		return DBSingleArr($result);
	}*/

	function DBFetchDeviceBySerialnumberAndUserId($serialnumber, $userid){
		if(!is_numeric($userid)){
			return array();
		}
		
		$userid = DBEscapeString($userid);
		$serialnumber = DBEscapeString($serialnumber);
		
		$result = DBFetch("useranddevice, devices", "devices.*,to_char(last_heard_of,'YY-MM-DD HH24:MI:ss') as last_checkin ", "useranddevice.device_id=devices.id AND useranddevice.user_id=$userid AND devices.serialnumber='$serialnumber'", "");
		return DBSingleArr($result);
	}
	
	function DBApproveDevice($id, $name){
		global $DB;
		$sth = $DB->prepare("update devices set approved='t', newborn='f', name=:name where id=:id");
		$result=$sth->execute(array(":id"=>$id,":name" => $name));
		
		if($result){
			return $sth->rowCount();
		}
		return -1;
	}
	
	function DBHideDevice(){
		global $DB;
		$sth = $DB->prepare("update devices set hide='t' where id=:id");
		$result = $sth->execute(array(":id" => $íd));
		
		if($result){
			return $sth->rowCount();
		}
		return -1;
		
	}
	
	function DBUpdateDevice($id, $name, $comments){
		global $DB;
		$sth = $DB->prepare("UPDATE devices SET name=:name, comments=:comments WHERE id=:id");
		$result = $sth->execute(array(":name" => $name, ":comments" => $comments, ":id" => $id));
		if(!$result){
			return 0;
		}
		return 1;
/*		$id = DBEscapeString($id);
		$name = DBEscapeString($name);
		
		$result = DBUpdate("devices", "name='$name'", "id=$id");
		return $result;*/
	}

	function DBUpdateDeviceLastHeardOf($id){
		if(!is_numeric($id)){
			return -1;
		}
		
		$id = DBEscapeString($id);
		$result = DBUpdate("devices", "last_heard_of=now()", "id=$id");
		return $result;
	}
	
	function DBUpdateDeviceLastHeardOfAndIPAddresses($id, $imscliversion, $publicip, $privateip1, $privateip2, $privateip3){
		if(!is_numeric($id)){
			return -1;
		}
		
		$id = DBEscapeString($id);
		$imscliversion = DBEscapeString($imscliversion);
		$publicip = DBEscapeString($publicip);
		$privateip1 = DBEscapeString($privateip1);
		$privateip2 = DBEscapeString($privateip2);
		$privateip3 = DBEscapeString($privateip3);
		
		$result = DBUpdate("devices", "last_heard_of=now(), imscliversion='$imscliversion', public_ip='$publicip', private_ip1='$privateip1', private_ip2='$privateip2', private_ip3='$privateip3'", "id=$id");
		return $result;
	}

	function DBUpdateDevicePublicKey($id, $publickey){
		if(!is_numeric($id)){
			return -1;
		}
		
		$id = DBEscapeString($id);
		$publickey = DBEscapeString($publickey);
		$result = DBUpdate("devices", "publickey='$publickey'", "id=$id");
		return $result;
	}
	
	function DBAddDevice($name, $serialnumber){
		$name = DBEscapeString($name);
		$serialnumber= DBEscapeString($serialnumber);
		
		$result = DBInsertAndGetID("devices", "name, serialnumber", "'$name', '$serialnumber'");		
		return $result;
	}
	
	function DBDeleteDevice($id){
		if(!is_numeric($id)){
			return -1;
		}
		
		$id = DBEscapeString($id);
		$result =DBDelete("devices", "id=$id");
	}
?>