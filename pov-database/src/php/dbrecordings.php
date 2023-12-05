<?php
    require_once("dblib.php");

	function DBFetchAllRecordings()
	{
		global $DB;
		
		$sth = $DB->prepare("select * from recordings order by name");
		$sth->execute(array());
		$result = $sth->fetchAll();
		if($result === false)
		{
			return array();
		}
		return $result;
		
	}
	
	function DBFetchRecordingById($id)
	{
		global $DB;
		
		$sth = $DB->prepare("select * from recordings where id=:id");
		$sth->execute(array(":id" => $id));
		$result = $sth->fetch();
		if($result === false)
        {
			return array();
		}
		return $result;
		
	}

	function DBFetchRecordingByName($name)
	{
		global $DB;
		
		$sth = $DB->prepare("select * from recordings where name=:name");
		$sth->execute(array(":name" => $name));
		$result = $sth->fetch();
		if($result === false)
        {
			return array();
		}
		return $result;
		
	}

	function DBFetchRecordingsByState($state)
	{
		global $DB;
		
		$sth = $DB->prepare("select * from recordings where state=:state");
		$sth->execute(array(":state" => $state));
		$result = $sth->fetchAll();
		if($result === false)
        {
			return array();
		}
		return $result;
		
	}
	
	
    function DBUpdateRecording($id, $streaminterfaceid, $name, $state, $recordingstart, $recordingend, $sourceaddress, $sourceport){
        global $DB;

		$streaminterfaceid = trim($streminterfaceid);
		if($streaminterfaceid == "")
		{
			return -1;
		}
        
		
		$sth = $DB->prepare("update recordings set streaminterfaceid=:streaminterfaceid, name=:name, state=:state, recordingstart=:recordingstart, recordingend=:recordingend, sourceaddress=s:sourceaddress, sourceport=:sourceportwhere id=:id");
		$result = $sth->execute(array(":streaminterfaceid" => $created,
									  ":name" => $expiration_days,
									  ":state" => $organisationid,
									  ":recordingstart" => $recordingstart,
									  ":recordingend" => $recordingend,
									  ":sourceaddress" => $sourceport,
									  ":sourceport" => $sourceport,
                                      ":id" => $id
								));
        if(!$result)
		{
            return 0;
        }
        return 1;
    }
	
    function DBAddRecording($streaminterfaceid, $name, $state, $recordingstart, $recordingend, $sourceaddress, $sourceport)
    {
        global $DB;

		$streaminterfaceid = trim($streaminterfaceid);
		if($streaminterfaceid == "")
        {
			return -1;
		}
        
		$sth = $DB->prepare("insert into recordings (streaminterface_id, name, state, recordingstart, recordingend, sourceaddress, sourceport) values (:streaminterfaceid, :name, :state, :recordingstart, :recordingend,:sourceaddress, :sourceport)");
		$result = $sth->execute(array(":streaminterfaceid" => $streaminterfaceid,
									  ":name" => $name,
									  ":state" => $state,
									  ":recordingstart" => $recordingstart,
									  ":recordingend" => $recordingend,
									  ":sourceaddress" => $sourceaddress,
									  ":sourceport" => $sourceport
								));
		if($result !== false)
        {
			return $DB->lastInsertId("recordings_id_seq");
		}
        return $result;
    }
    
    function DBDeleteRecordingById($id){
        global $DB;

        if(trim($id) == "")
		{
            return -1;
        }
        
        $sth = $DB->prepare("delete from recordings where id=:id");
        $sth->execute(array(":id" => $id));
		if($result)
		{
			return $sth->rowCount();
		}
        return -2;
    }
?>
