<?php
	require_once("dblib.php");

	function DBFetchSMBsetting()
	{
		global $DB;
		$sth = $DB->prepare("select * from smbsetting");
        	$sth->execute();
		$result = $sth->fetch();
		if($result === false){
			return array();
		}
		return $result;
	}

	function DBSetSMBinfo($mountfolder, $ipaddress, $sharename, $username)
	{
		global $DB;
		$sth = $DB->prepare("select * from smbsetting");
		$sth->execute();
		$rows = $sth->fetch();
		if($rows === false)
		{
			$di = $DB->prepare("insert into smbsetting (mountfolder, ipaddress, sharename, username) values (:mountfolder, :ipaddress, :sharename, :username)");

			$result = $di->execute(array(":mountfolder" => $mountfolder, ":ipaddress" => $ipaddress, ":sharename" => $sharename, ":username" => $username));
		}
		else
		{
			$dh = $DB->prepare("update smbsetting set mountfolder=:mountfolder, ipaddress=:ipaddress, sharename=:sharename, username=:username");

			$result = $dh->execute(array(":mountfolder" => $mountfolder, ":ipaddress" => $ipaddress, ":sharename" => $sharename, ":username" => $username));
		}
		if($result)
		{
			return 1;				
		}
		return 0;
	}

	function DBFetchSMBinfo()
	{
		global $DB;
		
		$sth = $DB->prepare("select mountfolder, ipaddress, sharename, username from smbsetting");
		$sth->execute(array());
		$result = $sth->fetch();
		
		if($result === false)
		{
			return array();
		}
		return $result;
	}
?>
