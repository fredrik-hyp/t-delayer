<?php
	require_once("dblib.php");
	class DBStreaminterfaceSink
	{
		protected $db = null;
		public $id = null;
		public $streaminterfaceconfig_id = null;
		public $name = null;
		public $priority = null;
		public $sinktype = null;
		public $host  = null;
		public $port = null;
		public $talkbackhost = null;
		public $talkbackport = null;
		public $ttl = null;
		public $hostclientcertificate_id = null;
		public $talkbackclientcertificate_id = null;
		public $maxretrytime = null;
		public $maxretries = null;
		public $streammissfailovertimeout = null;
		public $recording_id = null;
		public $rawudp = null;
		public $maxbps =null;
					
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
			isset($arr['sinktype']) ? $this->sinktype = $arr['sinktype'] : $this->sinktype = "rudp";
			isset($arr['host']) ? $this->host = $arr['host'] : $this->host = "";
			isset($arr['port']) ? $this->port = $arr['port'] : $this->port = null;
			isset($arr['talkbackhost']) ? $this->talkbackhost = $arr['talkbackhost'] : $this->talkbackhost = "";
			isset($arr['talkbackport']) ? $this->talkbackport = $arr['talkbackport'] : $this->talkbackport = null;
			isset($arr['ttl']) ? $this->ttl = $arr['ttl'] : $this->ttl = 1;
			isset($arr['hostclientcertificate_id']) ? $this->hostclientcertificate_id = $arr['hostclientcertificate_id'] : $this->hostclientcertificate_id = null;
			isset($arr['talkbackclientcertificate_id']) ? $this->talkbackclientcertificate_id = $arr['talkbackclientcertificate_id'] : $this->talkbackclientcertificate_id = null;
			isset($arr['maxretrytime']) ? $this->maxretrytime = $arr['maxretrytime'] : $this->maxretrytime = "";
			isset($arr['maxretries']) ? $this->maxretries = $arr['maxretries'] : $this->maxretries = "";
			isset($arr['streammissfailovertimeout']) ? $this->streammissfailovertimeout = $arr['streammissfailovertimeout'] : $this->streammissfailovertimeout = "0";
			isset($arr['recording_id']) ? $this->recording_id = $arr['recording_id'] : $this->recording_id = null;
			isset($arr['rawudp']) ? $this->rawudp = $arr['rawudp'] : $this->rawudp = "f";
			isset($arr['maxbps']) ? $this->maxbps = $arr['maxbps'] : $this->maxbps = 0;
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
			
			$sth = $this->db->prepare("select * from streaminterfacesinks where id=:id");
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
			if($streaminterfaceid < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("select * from streaminterfacesinks where streaminterfaceconfig_id=:streaminterfaceconfig_id");
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
			
			$sth = $this->db->prepare("update streaminterfacesinks set name = :name,
																		 priority = :priority, 
																		 sinktype = :sinktype,
																		 host = :host,
																		 port = :port,
																		 talkbackhost = :talkbackhost,
																		 talkbackport = :talkbackport,
																		 ttl = :ttl,
																		 maxbps = maxbps,
																		 hostclientcertificate_id = :hostclientcertificate_id,
																		 talkbackclientcertificate_id = :talkbackclientcertificate_id,
																		 maxretrytime = :maxretrytime,
																		 maxretries = :maxretries,
																		 streammissfailovertimeout = :streammissfailovertimeout,
																		 recording_id = :recording_id,
																		 rawudp = :rawudp,
																		 where id=:id");
			$result = $sth->execute(array(	":id" => $this->id,
											":name" => $name,
			 							   	":priority" => $this->priority,
											":sinktype" => $this->sinktype,
											":host" => $this->host,
											":port" => $this->port,
											":talkbackhost" => $this->talkbackhost,
											":talkbackport" => $this->talkbackport,
											":ttl" => $this->ttl,
											":maxbps" => $this->maxbps,
											":hostclientcertificate_id" => $this->hostclientcertificate_id,
											"talkbackclientcertificate_id" => $this->talkbackclientcertificate_id,
											":maxretrytime" => $this->maxretrytime,
											":maxretries" => $this->maxretries,
											":streammissfailovertimout" => $this->streammissfailovertimeout,
											":recording_id" => $this->recording_id,
											":rawudp" => $this->rawudp
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
			
			$sth = $this->db->prepare("delete from streaminterfacesinks where id=:id");
			$result = $sth->execute(array(":id" => $this->id));
		}
		
		function deleteBackupSinks()
		{
			if($this->streaminterfacebackupsink_id < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("delete from streaminterfacebackupsinks where streaminterfacesink_id=:streaminterfacesinkid");
			return $sth->execute(array(":streaminterfacebackupsinkid" => $this->streaminterfacebackupsink_id));
		}
	}
	
	function DBFetchStreaminterfaceSinkByStreaminterfaceConfigId($streaminterfaceconfigid){
		global $DB;
		
		$sth = $DB->prepare("select * from streaminterfacesinks where streaminterfaceconfig_id=:streaminterfaceconfigid");
		$sth->execute(array(":streaminterfaceconfigid" => $streaminterfaceconfigid));
		$result = $sth->fetch();
		if($result === false){
			return array();
		}
		return $result;
	}

	function DBUpdateStreaminterfaceSink($id, $name, $priority, $sinktype, $host, $port, $talkbackhost, $talkbackport, $ttl, $hostclientcertid, $talkbackclientcertid, $retrytime, $maxbps, $rawudp, $recordingid){
		global $DB;
		
		if(trim($port) == "" ||  strtolower($port)== "null"){
			$port=null;
		}
		if(trim($talkbackport) == "" ||  strtolower($talkbackport)== "null"){
			$talkbackport=null;
		}
		if(trim($hostclientcertid) == "" ||  strtolower($hostclientcertid) == "null"){
			$hostclientcertid= null;
		}

		if(trim($talkbackclientcertid) == "" ||  strtolower($talkbackclientcertid) == "null"){
			$talkbackclientcertid=null;
		}
        
        if(trim($retrytime) == "" ||  strtolower($retrytime) == "null"){
            $retrytime = null;
        }

		if(trim($maxbps) == "" ||  strtolower($maxbps)== "null"){
			$maxbps=null;
		}
        
		if(trim($recordingid) == "" ||  strtolower($recordingid)== "null"){
			$recordingid=null;
		}
		
		$sth = $DB->prepare("update streaminterfacesinks set name=:name, sinktype=:sinktype, host=:host, port=:port, talkbackhost=:talkbackhost, talkbackport=:talkbackport, ttl=:ttl, hostclientcertificate_id=:hostclientcertid, talkbackclientcertificate_id=:talkbackclientcertid, maxretrytime=:retrytime, maxbps=:maxbps, rawudp=:rawudp, recording_id=:recordingid where id=:id");
	/*	$sth = $DB->prepare("update streaminterfacesinks set name=:name, sinktype=:sinktype, host=:host, port=:port, talkbackhost=:talkbackhost, talkbackport=:talkbackport, ttl=:ttl where id=:id");
*/		
		$result = $sth->execute(array(
								":id" => $id,
								":name" => $name,
								":sinktype" => $sinktype,
								":host" => $host,
								":port" => $port,
								":talkbackhost" => $talkbackhost,
								":talkbackport" => $talkbackport,
								":ttl" => $ttl,
                                ":hostclientcertid" => $hostclientcertid,
                                ":talkbackclientcertid" => $talkbackclientcertid,
                                ":retrytime" => $retrytime,
								":maxbps" => $maxbps,
								":rawudp" => $rawudp,
								":recordingid" => $recordingid
								));
								
		if($result === false){
			return 0;
		}
		return 1;
	}
?>