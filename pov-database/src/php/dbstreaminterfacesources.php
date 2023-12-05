<?php
	require_once "dblib.php";
	class DBStreaminterfaceSource
	{
		protected $db = null;
		public $id = null;
		public $streaminterfaceconfig_id = null;
		public $name = null;
		public $priority = null;
		public $sourcetype = null;
		public $host  = null;
		public $port = null;
		public $talkbackhost = null;
		public $talkbackport = null;
		public $virtualsource_id = null;
		public $vsourcerevision = null;
		public $hostclientcertificate_id = null;
		public $talkbackclientcertificate_id = null;
		public $maxretrytime = null;
		public $maxretries = null;
		public $streammissfailovertimeout = null;
		public $recording_id = null;
		public $addrtp = null;
		
		function __construct($db, $id = null)
		{
			$this->db = $db;
			if($id != null)
			{
				$this->fetch($id);
			}
		}
		
		
		protected function copyRow($row)
		{
			foreach($row as $key => $element)
			{
				if(is_numeric($key)) continue;
				$this->${$key} = $element;
			}
		}
		
		function fromArray($arr)
		{
			if(!is_array($arr))
			{
				return false;
			}
			
			isset($arr['streaminterfaceconfig_id']) ? $this->streaminterfaceconfig_id = $arr['streaminterfaceconfig_id'] : $this->streaminterfaceconfig_id = null;
			isset($arr['name']) ? $this->name = $arr['name'] : $this->name = "";
			isset($arr['priority']) ? $this->priority = $arr['priority'] : $this->priority = 0;
			isset($arr['sourcetype']) ? $this->sourcetype = $arr['sourcetype'] : $this->sourcetype = "rudp";
			isset($arr['host']) ? $this->host = $arr['host'] : $this->host = "";
			isset($arr['port']) ? $this->port = $arr['port'] : $this->port = null;
			isset($arr['talkbackhost']) ? $this->talkbackhost = $arr['talkbackhost'] : $this->talkbackhost = "";
			isset($arr['talkbackport']) ? $this->talkbackport = $arr['talkbackport'] : $this->talkbackport = null;
			isset($arr['virtualsource_id']) ? $this->virtualsource_id = $arr['virtualsource_id'] : $this->virtualsource_id = null;
			isset($arr['vsourcerevision']) ? $this->vsourcerevision = $arr['vsourcerevision'] : $this->vsourcerevision = 0;
			isset($arr['hostclientcertificate_id']) ? $this->hostclientcertificate_id = $arr['hostclientcertificate_id'] : $this->hostclientcertificate_id = null;
			isset($arr['talkbackclientcertificate_id']) ? $this->talkbackclientcertificate_id = $arr['talkbackclientcertificate_id'] : $this->talkbackclientcertificate_id = null;
			isset($arr['maxretrytime']) ? $this->maxretrytime = $arr['maxretrytime'] : $this->maxretrytime = "";
			isset($arr['maxretries']) ? $this->maxretries = $arr['maxretries'] : $this->maxretries = "";
			isset($arr['streammissfailovertimeout']) ? $this->streammissfailovertimeout = $arr['streammissfailovertimeout'] : $this->streammissfailovertimeout = "0";
			isset($arr['recording_id']) ? $this->recording_id = $arr['recording_id'] : $this->recording_id = null;
			isset($arr['addrtp']) ? $this->addrtp = $arr['addrtp'] : $this->addrtp = "f";
			
			return true;
		}
		
		function toArray()
		{
			$result = array();
			$result['id'] = $this->id;
			return $result;
		}
		
		function fetch($id)
		{
			if($id < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("select * from streaminterfacesources where id=:id");
			$sth->execute(array(":id" => $id));
			$row = $sth->fetch();
			if($row === false){
				return false;
			}
			
			$this->copyRow($row);

			return true;
		}
			
		function fetchByStreaminterfaceConfigId($streaminterfaceconfigid)
		{
			if($streaminterfaceconfigid < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("select * from streaminterfacesources where streaminterfaceconfig_id=:streaminterfaceconfig_id");
			$sth->execute(array(":streaminterfaceconfig_id" => $streaminterfaceconfigid));
			$result = $sth->fetch();
			if($result === false){
				return false;
			}
			
			return $this->fromArray($result);			
		}
			
		function update()
		{
			if($this->id < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("update streaminterfacesources set name = :name,
																		 priority = :priority, 
																		 sourcetype = :sourcetype,
																		 host = :host,
																		 port = :port,
																		 talkbackhost = :talkbackhost,
																		 talkbackport = :talkbackport,
																		 virtualsource_id = :virtualsource_id,
																		 vsourcerevision = vsourcerevision,
																		 hostclientcertificate_id = :hostclientcertificate_id,
																		 talkbackclientcertificate_id = :talkbackclientcertificate_id,
																		 maxretrytime = :maxretrytime,
																		 maxretries = :maxretries,
																		 streammissfailovertimeout = streammissfailovertimeout,
																		 recording_id = :recording_id,
																		 addrtp = :addrtp,
																		 where id=:id");
			$result = $sth->execute(array(	":id" => $this->id,
											":name" => $name,
			 							   	":priority" => $this->priority,
											":sourcetype" => $this->sourcetype,
											":host" => $this->host,
											":port" => $this->port,
											":talkbackhost" => $this->talkbackhost,
											":talkbackport" => $this->talkbackport,
											":virtualsource_id" => $this->virtualsource_id,
											":vsourcerevision" => $this->vsourcerevision,
											":hostclientcertificate_id" => $this->hostclientcertificate_id,
											"talkbackclientcertificate_id" => $this->talkbackclientcertificate_id,
											":maxretrytime" => $this->maxretrytime,
											":maxretries" => $this->maxretries,
											":streammissfailovertimout" => $this->streammissfailovertimeout,
											":recording_id" => $this->recording_id,
											":addrtp" => $this->addrtp
										));

			if(!$result)
			{
				return false;
			}
			return true;
		}
		
		function delete()
		{
			if($this->id  < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("delete from streaminterfacesources where id=:id");
			$result = $sth->execute(array(":id" => $this->id));
			return $result;
		}
		
		function deleteBackupSources()
		{
			if($this->streaminterfacebackupsource_id < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("delete from streaminterfacebackupsources where streaminterfacesource_id=:streaminterfacesourceid");
			return $result = $sth->execute(array(":streaminterfacebackupsourceid" => $this->streaminterfacebackupsource_id));
		}
	}
	function DBFetchStreaminterfaceSourceByStreaminterfaceConfigId($streaminterfaceconfigid){
		global $DB;
		
		$sth = $DB->prepare("select * from streaminterfacesources where streaminterfaceconfig_id=:streaminterfaceconfigid");
		$sth->execute(array(":streaminterfaceconfigid" => $streaminterfaceconfigid));
		$result = $sth->fetch();
		if($result === false){
			return array();
		}
		return $result;
	}

	function DBUpdateStreaminterfaceSource($id, $name, $priority, $sourcetype, $host, $port, $talkbackhost, $talkbackport, $virtualsourceid, $hostclientcertid, $talkbackclientcertid, $retrytime, $streammiss, $recordingid=null, $addrtp = false){
		global $DB;

		if(trim($priority) == "" ||  strtolower($priority)== "null"){
			$priority = null;
		}
		
        if(trim($port) == "" || strtolower($port) == "null")
        {
            $port=null;
        }
        
		if(trim($talkbackport) == "" ||  strtolower($talkbackport)== "null"){
			$talkbackport = null;
		}
		
		if(trim($virtualsourceid) == "" || strtolower($virtualsourceid) == "null"){
			$virtualsourceid = null;
		}

		if(trim($hostclientcertid) == "" ||  strtolower($hostclientcertid) == "null"){
			$hostclientcertid = null;
		}

		if(trim($talkbackclientcertid) == "" || strtolower($talkbackclientcertid) == "null"){
			$talkbackclientcertid = null;
		}
		
        if(trim($retrytime) == "" || strtolower($retrytime) == "null"){
            $retrytime = null;
            
        }

        if(trim($streammiss) == "" || strtolower($streammiss) == "null"){
            $streammiss = null;
        }
        
        if(trim($recordingid) == "" || strtolower($recordingid) == "null"){
            $recordingid = null;
        }
        
		
		$sth = $DB->prepare("update streaminterfacesources set name=:name, priority=:priority, sourcetype=:sourcetype, host=:host, port=:port, talkbackhost=:talkbackhost, talkbackport=:talkbackport, virtualsource_id=:virtualsourceid, hostclientcertificate_id=:hostclientcertid, talkbackclientcertificate_id=:talkbackclientcertid, maxretrytime=:retrytime, streammissfailovertimeout=:streammiss, recording_id=:recordingid, addrtp=:addrtp where id=:id");
		
		$result = $sth->execute(array(
								":id" => $id,
								":name" => $name,
								":priority" => $priority,
								":sourcetype" => $sourcetype,
								":host" => $host,
								":port" => $port,
								":talkbackhost" => $talkbackhost,
								":talkbackport" => $talkbackport,
								":virtualsourceid" => $virtualsourceid,
                                ":hostclientcertid" => $hostclientcertid,
                                ":talkbackclientcertid" => $talkbackclientcertid,
                                ":retrytime" => $retrytime,
                                ":streammiss" => $streammiss,
								":recordingid" => $recordingid,
								":addrtp" => $addrtp
								));
								
		if($result === false){
			return 0;
		}
		return 1;
	}
?>