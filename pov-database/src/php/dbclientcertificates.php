<?php
    require_once("dblib.php");

    function DBFetchClientCertificateById($id){
        global $DB;

		$sth = $DB->prepare("select * from clientcertificates where id=:id");
		$sth->execute(array(":id" => $id));
		$result = $sth->fetch();
		if($result === false){
			return array();
		}
		return $result;
		
    }
    
    function DBFetchClientCertificatesByRootCertificateId($rootcertificateid){
        global $DB;

		$sth = $DB->prepare("select * from clientcertificates where rootcertificate_id=:rootcertificateid");
		$sth->execute(array(":rootcertificateid" => $rootcertificateid));
		$result = $sth->fetchAll();
		if($result === false){
			return array();
		}
		return $result;
		
    }
    function DBFetchClientCertificateByNameAndRootCertificateId($name, $rootcertificateid){
        global $DB;

		$sth = $DB->prepare("select * from clientcertificates where name=:name and rootcertificate_id=:rootcertificateid");
		$sth->execute(array(":name" => $name,
                            ":rootcertificateid" => $rootcertificateid));
		$result = $sth->fetch();
		if($result === false){
			return array();
		}
		return $result;
    }
    
    function DBAddClientCertificate($createdat="", $expiration_days="", $rootcertificateid, $name, $public_part, $private_part, $notbefore, $notafter)
	{
        global $DB;

		$rootcertificateid = trim($rootcertificateid);
		if($rootcertificateid == "")
		{
			return -1;
		}
        
        if(trim($createdat) == "")
		{
            $createdat = "now()";
        }
        
        if(trim($expiration_days) == "")
		{
            $expiration_days = 0;
        }
        
        $server="t";
        if(trim($private_part) == "")
		{
            $server="f";
        }
		
		$sth = $DB->prepare("insert into clientcertificates (created, expiration_days, rootcertificate_id, name, public_part, private_part,server,notbefore, notafter) values (:createdat, :expiration_days, :rootcertificateid, :name,:public_part, :private_part, :server, :notbefore, :notafter)");
		$result = $sth->execute(array(":createdat" => $createdat,
									  ":expiration_days" => $expiration_days,
									  ":rootcertificateid" => $rootcertificateid,
									  ":name" => $name,
									  ":public_part" => $public_part,
									  ":private_part" => $private_part,
                                      ":server" => $server,
									  ":notbefore" => $notbefore,
									  ":notafter" => $notafter
								));
		if($result)
		{
			return $DB->lastInsertId("clientcertificates_id_seq");
		}
        return $result;
    }

    function DBUpdateClientCertificate($id, $createdat="", $expiration_days="", $rootcertificateid, $name, $public_part, $private_part, $notbefore, $notafter)
	{
        global $DB;

        if(!is_numeric($id))
		{
            return -1;
        }

		$roocertificateid = trim($rootcertificateid);
		if($rootcertificateid == "")
		{
			return -2;
		}

        if(trim($createdat) == "")
		{
            $createdat = "now()";
        }
        if(trim($expiration_days) == "")
		{
            $expiration_days = 0;
        }
        
        $server="t";
        if(trim($private_part) == "")
		{
            $server="f";
        }
        
		$sth = $DB->prepare("update clientcertificates set created=:createdat, expiration_days=:expiration_days, rootcertificate_id=:rootcertificateid, name=:name, public_part=:public_part, private_part=:private_part, server=:server, notbefore=:notbefore, notafter=:notafter where id=:id");
		$result = $sth->execute(array(":createdat" => $createdat,
									  ":expiration_days" => $expiration_days,
									  ":rootcertificateid" => $rootcertificateid,
									  ":name" => $name,
									  ":public_part" => $public_part,
									  ":private_part" => $private_part,
                                      ":server" => $server,
									  ":notbefore" => $notbefore,
									  ":notafter" => $notafter,
                                      ":id" => $id
								));
        if(!$result)
		{
            return 0;
        }
        return $result;
    }
    
    function DBDeleteClientCertificateById($id)
	{
        global $DB;
        if(trim($id) == "")
		{
            return -1;
        }
        
        $sth = $DB->prepare("delete from clientcertificates where id=:id");
        $result = $sth->execute(array(":id" => $id));
		if($result)
		{
			return $sth->rowCount();
		}
        return -2;
    }

?>