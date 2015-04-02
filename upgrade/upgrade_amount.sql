-- update record_item.amount
ALTER TABLE record_item ALTER COLUMN amount TYPE numeric(10,4);

-- new VAT related tables
CREATE TABLE lov_vat
(
   id     serial         NOT NULL,
   value  numeric(8,2)   NOT NULL,
   label  varchar(20)    NOT NULL,
   note   varchar(100)
);
ALTER TABLE lov_vat ADD CONSTRAINT lov_vat_pkey PRIMARY KEY (id);
ALTER TABLE lov_vat ADD CONSTRAINT lov_vat_label_key UNIQUE (label);

CREATE TABLE lov_product_vat
(
   id          serial    NOT NULL,
   prod_id     integer    NOT NULL,
   vat_id      integer    NOT NULL,
   valid_from  date       NOT NULL,
   valid_to    date
);

ALTER TABLE lov_product_vat ADD CONSTRAINT lov_product_vat_pkey PRIMARY KEY (id);
ALTER TABLE lov_product_vat
  ADD CONSTRAINT fk_product_vat$prod_id FOREIGN KEY (prod_id)
  REFERENCES lov_product (id)
  ON UPDATE NO ACTION
  ON DELETE NO ACTION;
ALTER TABLE lov_product_vat
  ADD CONSTRAINT fk_product_vat$vat_id FOREIGN KEY (vat_id)
  REFERENCES lov_vat (id)
  ON UPDATE NO ACTION
  ON DELETE NO ACTION;
  
create index idx_lov_product_vat$prod_id on lov_product_vat(prod_id);
-- create index idx_lov_product_vat$vat_id on lov_product_vat(vat_id); -- is it necessary ???
  
-- default migration to new tables
insert into lov_vat (value, label, note) values (0.00, '0%', 'DEFAULT NO VAT');
-- czech VAT
--insert into lov_vat (value, label, note) values (0.21, '21%', 'Zakladní sadzba');
--insert into lov_vat (value, label, note) values (0.15, '15%', 'Snížená sazba');
--insert into lov_vat (value, label, note) values (0.10, '10%', 'Druhá snížená sazba');

DO language plpgsql $$
DECLARE
    prod RECORD;
BEGIN
	FOR prod IN select ID FROM LOV_PRODUCT ORDER BY ID 
	LOOP
		-- default
		EXECUTE 'INSERT INTO lov_product_vat (prod_id, vat_id, valid_from) VALUES (' || prod.id ||  ', ' || (select ID from lov_vat where label = '0%') || ', DATE ''2000-01-01'')';
		-- czech
		-- EXECUTE 'INSERT INTO lov_product_vat (prod_id, vat_id, valid_from, valid_to) VALUES (' || prod.id ||  ', ' || (select ID from lov_vat where label = '0%') || ', DATE ''2000-01-01'', DATE ''2015-01-31'')';
		-- EXECUTE 'INSERT INTO lov_product_vat (prod_id, vat_id, valid_from) VALUES (' || prod.id ||  ', ' || (select ID from lov_vat where label = '21%') || ', DATE ''2015-02-01'')';
	END LOOP;
END
$$;

-- TODO recalculate product price according to current VAT


-- migrate old record items to new VAT-enabled products
ALTER TABLE RECORD_ITEM ADD COLUMN new_prod_id integer;
ALTER TABLE RECORD_ITEM
  ADD CONSTRAINT fk_record_item$prod_vat_id FOREIGN KEY (new_prod_id)
  REFERENCES lov_product_vat (id)
  ON UPDATE NO ACTION
  ON DELETE NO ACTION;

DO language plpgsql $$
DECLARE
    rec RECORD;
BEGIN
	FOR rec IN select ID, PROD_ID FROM RECORD_ITEM ORDER BY ID 
	LOOP
		-- !!! Fix selected VAT, if necessary
		EXECUTE 'UPDATE RECORD_ITEM SET NEW_PROD_ID = (SELECT ID FROM lov_product_vat where PROD_ID = ' || rec.PROD_ID || ' and vat_id = (select ID from lov_vat where label = ''0%'')) where ID = ' || rec.ID;
	END LOOP;
END
$$;
  
ALTER TABLE RECORD_ITEM ALTER COLUMN new_prod_id SET NOT NULL;
ALTER TABLE RECORD_ITEM RENAME COLUMN prod_id to PROD_ID_OLD;
ALTER TABLE RECORD_ITEM RENAME COLUMN new_prod_id to PROD_ID;
ALTER TABLE RECORD_ITEM ALTER COLUMN prod_id_old DROP NOT NULL;

-- functions for VAT calculation
CREATE OR REPLACE FUNCTION CALCULATE_VAT(item_price NUMERIC(8,2), vat_rate NUMERIC(8,2))
RETURNS NUMERIC(8,2) AS $$
BEGIN
	RETURN ROUND(NUMERIC_MUL(item_price, vat_rate), 2);
END;
$$  LANGUAGE plpgsql IMMUTABLE

CREATE OR REPLACE FUNCTION CALCULATE_PRICE(item_price NUMERIC(8,2), vat_rate NUMERIC(8,2))
RETURNS NUMERIC(8,2) AS $$
BEGIN
	RETURN ROUND(NUMERIC_MUL(item_price, vat_rate), 2) + item_price;
END;
$$  LANGUAGE plpgsql IMMUTABLE

