<?php
    require_once("dblib.php");
	class DBStreaminterfaceBackupSource
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
		public $hostclientcertificate_id = null;
		public $talkbackclientcertificate_id = null;
		public $maxretrytime = null;
		public $maxretries = null;
		public $streammissfailovertimeout = null;
		
		public $DBStreaminterfaceBackupSources = array();
		
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
			isset($arr['hostclientcertificate_id']) ? $this->hostclientcertificate_id = $arr['hostclientcertificate_id'] : $this->hostclientcertificate_id = null;
			isset($arr['talkbackclientcertificate_id']) ? $this->talkbackclientcertificate_id = $arr['talkbackclientcertificate_id'] : $this->talkbackclientcertificate_id = null;
			isset($arr['maxretrytime']) ? $this->maxretrytime = $arr['maxretrytime'] : $this->maxretrytime = "";
			isset($arr['maxretries']) ? $this->maxretries = $arr['maxretries'] : $this->maxretries = "";
			isset($arr['streammissfailovertimeout']) ? $this->streammissfailovertimeout = $arr['streammissfailovertimeout'] : $this->streammissfailovertimeout = "0";
			
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
			
		function fetchByStreaminterfaceSourceId($streaminterfacesourceid)
		{
			if($streaminterfacesourceid < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("select * from streaminterfacebackupsources where streaminterfaceconfig_id=:streaminterfacesource_id order by priority");
			$sth->execute(array(":streaminterfacecsource_id" => $streaminterfacesourceid));
			$result = $sth->fetch();
			if($result === false){
				return false;
			}
			
			$this->StreaminterfaceBackupSources = array();
			foreach($result as $source)
			{
				$sourcenew = new DBStreaminterfaceBackupSource($this->db);
				$sourcenew->fromArray($source);
				$this->StreaminterfaceBackupSources[] = $sourcenew;
			}
			return 	true;
		}
		
		function insert()
		{
			$sth = $this->db->prepare("insert into streaminterfacebackupsources (name, priority, sourcetype, host, port, talkbackhost,
																				talbackport, hostclientcertificate_id, talkbackclientcertificate_id,
																				maxretrytime, maxretries, streammissfailovertimeout)
																				VALUES 
																				(:name, :priority, :sourcetype, :host, :port, :talkbackhost,
																					:talkbackport, :hostclientcertificate_id, :maxretrytime,
																					:maxretries, :streammissfailovertimeout)");
			$result = $sth->execute(array(":name" => $name,
			 							   	":priority" => $this->priority,
											":sourcetype" => $this->sourcetype,
											":host" => $this->host,
											":port" => $this->port,
											":talkbackhost" => $this->talkbackhost,
											":talkbackport" => $this->talkbackport,
											":hostclientcertificate_id" => $this->hostclientcertificate_id,
											"talkbackclientcertificate_id" => $this->talkbackclientcertificate_id,
											":maxretrytime" => $this->maxretrytime,
											":maxretries" => $this->maxretries,
											":streammissfailovertimout" => $this->streammissfailovertimeout));
			if($result)
			{
				return $this->db->lastInsertId("streaminterfacebackupsources_id_seq");
			}
			return $result;
		}
		
		function update()
		{
			if($this->id < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("update streaminterfacebackupsources set name = :name,
																		 priority = :priority, 
																		 sourcetype = :sourcetype,
																		 host = :host,
																		 port = :port,
																		 talkbackhost = :talkbackhost,
																		 talkbackport = :talkbackport,
																		 hostclientcertificate_id = :hostclientcertificate_id,
																		 talkbackclientcertificate_id = :talkbackclientcertificate_id,
																		 maxretrytime = :maxretrytime,
																		 maxretries = :maxretries,
																		 streammissfailovertimeout = streammissfailovertimeout,
																		 where id=:id");
			$result = $sth->execute(array(	":id" => $this->id,
											":name" => $name,
			 							   	":priority" => $this->priority,
											":sourcetype" => $this->sourcetype,
											":host" => $this->host,
											":port" => $this->port,
											":talkbackhost" => $this->talkbackhost,
											":talkbackport" => $this->talkbackport,
											":hostclientcertificate_id" => $this->hostclientcertificate_id,
											"talkbackclientcertificate_id" => $this->talkbackclientcertificate_id,
											":maxretrytime" => $this->maxretrytime,
											":maxretries" => $this->maxretries,
											":streammissfailovertimout" => $this->streammissfailovertimeout
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
			
			$sth = $this->db->prepare("delete from streaminterfacebackupsources where id=:id");
			$result = $sth->execute(array(":id" => $this->id));
			return $result;
		}
	}
	
    function DBFetchStreaminterfaceBackupSourcesByStreaminterfacesourceId($streaminterfacesourceid){
        global $DB;
        
        $sth = $DB->prepare("select * from streaminterfacebackupsources where streaminterfacesource_id=:streaminterfacesourceid order by priority");
		$sth->execute(array(":streaminterfacesourceid" => $streaminterfacesourceid));

		$result = $sth->fetchAll();
		if($result === false){
			return array();
		}
		return $result;
    }

    function DBUpdateStreaminterfaceBackupSource($id, $name, $priority, $maxretrytime, $streammissfailovertimeout, $sourcetype, $host, $port, $talkbackhost, $talkbackport, $hostclientcertificateid, $talkbackclientcertificateid){
        global $DB;
        
        $sth = $DB->prepare("update streaminterfacebackupsources set
                                name=:name,
                                priority=:priority,
                                maxretrytime=:maxretrytime, 
                                streammissfailovertimeout=:streammissfailovertimeout,
                                sourcetype=:sourcetype,
                                host=:host,
                                port=:port,
                                talkbackhost=:talkbackhost,
                                talkbackport=:talkbackport,
                                hostclientcertificate_id=:hostclientcertificateid,
                                talkbackclientcertificate_id=:talkbackclientcertificateid
                                where id=:id");
                                
        $result = $sth->execute(array(":name" => $name,
                        ":priority" => $priority,
                        ":maxretrytime" => $maxretrytime,
                        ":streammissfailovertimeout" => $streammissfailovertimeout,
                        ":sourcetype" => $sourcetype,
                        ":host" => $host,
                        ":port" => $port,
                        ":talkbackhost" => $talkbackhost,
                        ":talkbackport" => $talkbackport,
                        ":hostclientcertificateid" => $hostclientcertificateid,
                        ":talkbackclientcertificateid" => $talkbackclientcertificateid,
                        ":id" => $id
                    ));
                    
        if(!$result){
            return 0;
        }
        return 1;
    }

    function DBAddStreaminterfaceBackupSource($streaminterfacesourceid, $name, $priority, $maxretrytime, $streammissfailovertimeout, $sourcetype, $host, $port, $talkbackhost, $talkbackport, $hostclientcertificateid, $talkbackclientcertificateid){
        global $DB;
        
        if(!is_numeric($streaminterfacesourceid) || trim($streaminterfacesourceid) == "" ){
            return -1;
        }
        
        if(!is_numeric($port) || trim($port) == ""){
            $port = 0;
        }
        
        if(!is_numeric($talkbackport) || trim($talkbackport) == ""){
            $talkbackport=0;
        }
        
        if(!is_numeric($maxretrytime) || trim($maxretrytime) == ""){
            $maxretrytime =0;
        }
        
        if(!is_numeric($streammissfailovertimeout) || trim($streammissfailovertimeout) == ""){
            $streammissfailovertimeout = 0;
        }
        if(trim($hostclientcertificateid) == "" || $hostclientcertificateid == "null"){
            $hostclientcertificateid = null;
        }
        
        if(trim($talkbackclientcertificateid) == "" || $talkbackclientcertificateid == "null"){
            $talkbackclientcertificateid = null;
        }
        
        $sth = $DB->prepare("insert into streaminterfacebackupsources (streaminterfacesource_id, name, priority, maxretrytime, streammissfailovertimeout, sourcetype, host, port, talkbackhost, talkbackport, hostclientcertificate_id, talkbackclientcertificate_id) values (:streaminterfacesourceid, :name, :priority, :maxretrytime, :streammissfailovertimeout, :sourcetype, :host, :port, :talkbackhost, :talkbackport, :hostclientcertificateid, :talkbackclientcertificateid)");
        
        $result = $sth->execute(array(":streaminterfacesourceid" => $streaminterfacesourceid,
                                    ":name" => $name,
                                    ":priority" => $priority,
                                    ":maxretrytime" => $maxretrytime,
                                    ":streammissfailovertimeout" => $streammissfailovertimeout,
                                    ":sourcetype" => $sourcetype,
                                    ":host" => $host,
                                    ":port" => $port,
                                    ":talkbackhost" => $talkbackhost,
                                    ":talkbackport" => $talkbackport,
                                    ":hostclientcertificateid" => $hostclientcertificateid,
                                    ":talkbackclientcertificateid" => $talkbackclientcertificateid
                                ));

        if($result){
        	return $DB->lastInsertId("streaminterfacebackupsources_id_seq");
        }
        return $result;
    }
        
    function DBDeletestreaminterfacebackupsourceByStreaminterfacesourceId($streaminterfacesourceid){
        global $DB;
        
        if(trim($streaminterfacesourceid) == ""){
            return -1;
        }
        
        $sth = $DB->prepare("delete from streaminterfacebackupsources where streaminterfacesource_id=:streaminterfacesourceid");
        $result = $sth->execute(array(":streaminterfacesourceid" => $streaminterfacesourceid));
        
        if($result){
            return $sth->rowCount();
        }
        
        return -2;
    }
?>