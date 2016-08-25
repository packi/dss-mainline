begin transaction;
drop table if exists "foo";
create table "foo"(id primary key,name);
insert into foo values(7 , "bar", "ips");
commit;
