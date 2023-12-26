CREATE TABLE IF NOT EXISTS numbers (
    tag text,
    product text,
    gitcommit text,
    epoch integer,
    benchmark text,
    repeat text,
    measurement text,
    scale integer,
    ns real,
    primary key (tag, product, gitcommit, epoch, benchmark, repeat, measurement, scale)
);
CREATE INDEX if not exists ix_numbers_tag on numbers(tag);
CREATE INDEX if not exists ix_numbers_gitcommit on numbers(gitcommit);
CREATE INDEX if not exists ix_numbers_benchmark on numbers(benchmark);
CREATE INDEX if not exists ix_numbers_measurement on numbers(measurement);
CREATE INDEX if not exists ix_numbers_scale on numbers(scale);
DROP VIEW IF EXISTS summary_stats;
CREATE VIEW summary_stats as
select
    tag, 
    product,
    gitcommit,
    benchmark,
    measurement,
    scale,
    avg(ns) as mean,
    avg((ns - sub.mu) * (ns - sub.mu)) as var,
    min(ns) as min,
    max(ns) as max
from
    numbers,
    (select avg(ns) as mu from numbers) as sub
group by tag, product, gitcommit, benchmark, measurement, scale;
