# postgresql_db

This role ensures a database exists within a PostgreSQL DBMS. The role assumes the database has an
admin user.

## Requirements

None

## Role Variables

Variable                       | Required | Default  | Choices | Comment
------------------------------ | -------- | -------- | ------- | -------
`postgresql_db_dbms_port`      | no       | 5432     |         | the TCP the DBMS is listening on
`postgresql_db_name`           | no       | postgres |         | the name of the database
`postgresql_db_admin_user`     | no       | postgres |         | the account name of the database admin
`postgresql_db_admin_password` | yes      |          |         | the password used to authenticate the database's admin user

## Dependencies

None

## Example Playbook

```yaml
- hosts: dbms
  roles:
    - role: postgresql_db
      vars:
        postgresql_db_name: ICAT
        postgresql_db_admin_user: irods
        postgresql_db_admin_password: testpassword
```
